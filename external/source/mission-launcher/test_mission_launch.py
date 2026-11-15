import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from mission_launch import Scanner, make_config, launch, root, field, write_collection, tokens


class MissionDependencyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gc-launch-test-")
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.projects = []
        self.ids = {}
        for index, name in enumerate(("Mission", "Terrain", "Units", "Weapons", "Attachments", "Unused"), 1):
            addon = f"{index:016X}"
            self.ids[name] = addon
            directory = self.folder / name
            directory.mkdir()
            project = directory / "addon.gproj"
            project.write_text(f'GameProject {{ ID "{name}" GUID "{addon}" TITLE "{name}" Dependencies {{ "58D0FB3206B6F859" }} }}')
            (directory / "ServerData.json").write_text(json.dumps({"id": addon, "revision": {"version": "1.0.0", "dependencies": []}}))
            self.projects.append(project)
        self.attachment = self.resource("Attachments", "optic.et", 106, 'Optic {}')
        self.weapon = self.resource("Weapons", "rifle.et", 105, f'Weapon {{ Optic "{self.attachment}" }}')
        self.unit = self.resource("Units", "soldier.et", 104, f'Unit {{ Inventory {{ "{self.weapon}" }} }}')
        self.terrain = self.resource("Terrain", "worlds/island.ent", 103, 'World {}')
        self.world = self.resource("Mission", "worlds/operation.ent", 102, f'SubScene {{ Parent "{self.terrain}" }}')
        layers = self.folder / "Mission/worlds/operation_Layers"
        layers.mkdir()
        (layers / "default.layer").write_text(f'Layer {{ Unit "{self.unit}" }}')
        self.scenario = self.resource("Mission", "Missions/operation.conf", 101, f'Header {{ World "{self.world}" }}')
        self.mission = self.folder / "Mission/Missions/operation.conf"
        self.template = {"bindPort": 29999, "game": {"name": "Private prototype", "password": "preserve-me", "visible": False, "mods": [{"modId": self.ids["Unused"]}]}}

    def resource(self, owner, relative, number, text):
        target = self.folder / owner / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        identity = f"{{{number:016X}}}{relative}"
        Path(str(target) + ".meta").write_text(f'MetaFileClass {{ Name "{identity}" }}')
        return identity

    def scan(self):
        return Scanner(self.projects).scan(self.mission)

    def test_terrain_layers_inventory_weapon_attachment_and_unused(self):
        result = self.scan()
        self.assertEqual(result["issues"], [])
        self.assertEqual({m["modId"] for m in result["mods"]}, {self.ids[n] for n in self.ids if n != "Unused"})
        self.assertTrue(any(e["from"] == self.weapon and e["to"] == self.attachment for e in result["edges"]))
        config = make_config(result, self.template)
        self.assertEqual(config["game"]["scenarioId"], self.scenario)
        self.assertEqual(config["game"]["password"], "preserve-me")
        self.assertEqual(config["bindPort"], 29999)
        self.assertEqual(len(self.template["game"]["mods"]), 1)
        self.assertNotIn(self.ids["Unused"], [m["modId"] for m in config["game"]["mods"]])

    def test_native_dependencies_override_empty_server_data(self):
        project = self.projects[4]
        project.write_text(project.read_text().replace('"58D0FB3206B6F859"', f'"58D0FB3206B6F859" "{self.ids["Unused"]}"'))
        result = self.scan()
        self.assertEqual(len(result["mods"]), 6)
        self.assertIn(self.ids["Attachments"], next(m for m in result["mods"] if m["modId"] == self.ids["Unused"])["reasons"])

    def test_project_configuration_resource_is_not_omitted(self):
        setting = self.resource("Unused", "Configs/shared.conf", 112, 'Settings {}')
        project = self.projects[4]
        project.write_text(project.read_text().replace(' TITLE ', f' Settings "{setting}" TITLE '))
        result = self.scan()
        self.assertEqual(len(result["mods"]), 6)
        self.assertTrue(any(e["to"] == setting and e["from"].startswith("project:") for e in result["edges"]))

    def test_core_system_selected_without_any_mission_asset_reference(self):
        result = Scanner(self.projects).scan(self.mission, [self.ids["Unused"], self.ids["Unused"]])
        self.assertEqual(result["coreAddonIds"], [self.ids["Unused"]])
        core = next(m for m in result["mods"] if m["modId"] == self.ids["Unused"])
        self.assertIn("GC core system policy", core["reasons"])
        self.assertEqual(len(make_config(result, self.template)["game"]["mods"]), 6)

    def test_core_selection_does_not_bypass_missing_or_script_coverage(self):
        missing = Scanner(self.projects).scan(self.mission, ["FFFFFFFFFFFFFFFF"])
        self.assertTrue(any("Missing addon" in issue for issue in missing["issues"]))
        script = self.folder / "Unused/scripts/Game/Feature.c"
        script.parent.mkdir(parents=True)
        script.write_text('class Feature {}')
        result = Scanner(self.projects).scan(self.mission, [self.ids["Unused"]])
        self.assertTrue(any("Runtime script" in issue for issue in result["issues"]))
        with self.assertRaisesRegex(ValueError, "incomplete"):
            make_config(result, self.template)

    def test_missing_attachment_blocks_config(self):
        (self.folder / "Attachments/optic.et").unlink()
        result = self.scan()
        self.assertTrue(any("missing resource" in s for s in result["issues"]))
        with self.assertRaisesRegex(ValueError, "incomplete"):
            make_config(result, self.template)

    def test_missing_terrain_blocks(self):
        Path(str(self.folder / "Terrain/worlds/island.ent") + ".meta").unlink()
        self.assertTrue(any(self.terrain in s for s in self.scan()["issues"]))

    def test_inherited_prefab_and_random_variants_are_all_followed(self):
        other = self.resource("Unused", "alternative.et", 107, 'Unit {}')
        (self.folder / "Units/soldier.et").write_text(f'Unit : "{self.weapon}" {{ Variants {{ "{self.weapon}" "{other}" }} }}')
        self.assertEqual(len(self.scan()["mods"]), 6)

    def test_resource_cycle_terminates(self):
        (self.folder / "Attachments/optic.et").write_text(f'Optic {{ Parent "{self.weapon}" }}')
        self.assertEqual(self.scan()["status"], "static-complete")

    def test_dependency_cycle_is_rejected(self):
        self.projects[4].write_text(self.projects[4].read_text().replace('"58D0FB3206B6F859"', f'"{self.ids["Attachments"]}"'))
        self.assertTrue(any("dependency cycle" in s for s in self.scan()["issues"]))

    def test_changed_attachment_and_added_layer_invalidate_metadata(self):
        result = self.scan()
        (self.folder / "Attachments/optic.et").write_text('Optic { Changed 1 }')
        with self.assertRaisesRegex(ValueError, "Stale"):
            make_config(result, self.template)
        result = self.scan()
        (self.folder / "Mission/worlds/operation_Layers/new.layer").write_text('Layer {}')
        with self.assertRaisesRegex(ValueError, "file set changed"):
            make_config(result, self.template)

    def test_new_script_invalidates_metadata(self):
        result = self.scan()
        script = self.folder / "Units/scripts/Game/Spawner.c"
        script.parent.mkdir(parents=True)
        script.write_text('class Spawner {}')
        with self.assertRaisesRegex(ValueError, "file set changed"):
            make_config(result, self.template)
        self.assertTrue(any("Runtime script" in s for s in self.scan()["issues"]))

    def test_opaque_binary_and_path_only_references_block(self):
        binary = self.resource("Attachments", "mesh.xob", 108, 'not a parsed mesh')
        (self.folder / "Attachments/optic.et").write_text(f'Optic {{ Mesh "{binary}" Other "Prefabs/unknown.et" }}')
        issues = self.scan()["issues"]
        self.assertTrue(any("Unsupported resource format" in s for s in issues))
        self.assertTrue(any("Unqualified resource" in s for s in issues))

    def test_path_only_material_and_animation_block(self):
        (self.folder / "Attachments/optic.et").write_text('Optic { Material "Materials/missing.gamemat" Animation "move.anm" }')
        issues = self.scan()["issues"]
        self.assertTrue(any("missing.gamemat" in s for s in issues))
        self.assertTrue(any("move.anm" in s for s in issues))

    def test_new_archive_or_changed_unused_metadata_invalidates(self):
        result = self.scan()
        (self.folder / "Attachments/new.pak").touch()
        with self.assertRaisesRegex(ValueError, "archive set changed"):
            make_config(result, self.template)
        self.assertTrue(any("Packed addon" in s for s in self.scan()["issues"]))
        (self.folder / "Attachments/new.pak").unlink()
        self.resource("Unused", "potential_override.et", 111, 'Optic {}')
        result = self.scan()
        meta = self.folder / "Unused/potential_override.et.meta"
        meta.write_text(meta.read_text().replace("000000000000006F", "000000000000006A"))
        with self.assertRaisesRegex(ValueError, "Stale"):
            make_config(result, self.template)

    def test_comments_and_strings_cannot_inject_dependencies(self):
        project = self.projects[0]
        project.write_text(project.read_text() + '\n// Dependencies { "FFFFFFFFFFFFFFFF" }\n')
        self.assertEqual(len(self.scan()["mods"]), 5)
        with self.assertRaisesRegex(ValueError, "Unterminated"):
            tokens('Object { /* unterminated')
        with self.assertRaisesRegex(ValueError, "Binary data"):
            tokens('World {\0}')

    def test_collection_has_independent_identity_and_preserves_inputs(self):
        before = [p.read_bytes() for p in self.projects]
        output = self.folder / "collection.gproj"
        count = write_collection(self.projects, "FFFFFFFFFFFFFFFF", output)
        _, body = root(output)
        self.assertEqual(field(body, "ID"), "GlobalConflictsWorkbenchDev")
        self.assertEqual(count, 7)
        self.assertEqual(before, [p.read_bytes() for p in self.projects])
        with self.assertRaisesRegex(ValueError, "own new addon identity"):
            write_collection(self.projects, self.ids["Mission"], self.folder / "bad.gproj")

    def test_collection_and_duplicate_override_block(self):
        self.projects[4].write_text(self.projects[4].read_text().replace('ID "Attachments"', 'ID "GlobalConflictsWorkbenchDev"'))
        self.assertTrue(any("Authoring collection" in s for s in self.scan()["issues"]))
        self.resource("Unused", "optic.et", 106, 'Optic {}')
        self.assertTrue(any("Ambiguous" in s for s in self.scan()["issues"]))

    def test_generated_snapshot_does_not_select_unrelated_mod(self):
        unrelated = self.resource("Unused", "unrelated.et", 110, 'Object {}')
        self.mission.write_text(f'Header {{ World "{self.world}" m_GC_GeneratedMetadata Snapshot {{ Resource "{unrelated}" }} }}')
        self.assertEqual(len(self.scan()["mods"]), 5)

    def test_launch_uses_separate_arguments_and_fresh_profile(self):
        config = make_config(self.scan(), self.template)
        fake_exe = self.folder / "server executable.exe"
        fake_exe.touch()
        profile = self.folder / "isolated profile"
        with patch("mission_launch.subprocess.Popen") as start:
            start.return_value.wait.return_value = 0
            self.assertEqual(launch(config, fake_exe, profile, self.folder / "server.json"), 0)
            args = start.call_args.args[0]
            self.assertEqual(args[0], str(fake_exe))
            self.assertEqual(args[args.index("-profile") + 1], str(profile))
            self.assertFalse(start.call_args.kwargs.get("shell", False))
        with self.assertRaises(FileExistsError):
            launch(config, fake_exe, profile, self.folder / "second.json")


if __name__ == "__main__":
    unittest.main()
