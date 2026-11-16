"""Development checks for the GC addon split; never mutates addon projects."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def project(path):
    text = path.read_text(encoding="utf-8-sig")
    guid = re.search(r'\bGUID\s+"([A-F0-9]{16})"', text).group(1)
    body = re.search(r'\bDependencies\s*\{([^}]*)}', text).group(1)
    dependencies = re.findall(r'"([A-F0-9]{16})"', body)
    assert len(dependencies) == len(set(dependencies)), f"Duplicate dependency: {path}"
    return guid, dependencies


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--addons-dir", type=Path, default=here.parents[3])
    parser.add_argument("--installed-addons", type=Path, default=Path.home() / "Documents/My Games/ArmaReforger/addons")
    args = parser.parse_args()
    root = args.addons_dir.resolve()
    local = {name: root / f"gc-reforger-{name}" / "addon.gproj" for name in ("core", "units", "missions")}
    local["lobby"] = root / "gc-lobby/addon.gproj"
    local["workshop"] = root / "gc-reforger-workshop/addon.gproj"
    graph = {}
    for path in args.installed_addons.glob("*/*.gproj"):
        guid, requirements = project(path)
        graph[guid] = requirements
    ids = {}
    for name, path in local.items():
        guid, requirements = project(path)
        ids[name] = guid
        graph[guid] = requirements
    vanilla = {"58D0FB3206B6F859", "core"}

    def closure(guid, ancestors=()):
        assert guid not in ancestors, f"Dependency cycle: {ancestors + (guid,)}"
        if guid in vanilla:
            return {guid}
        assert guid in graph, f"Missing project: {guid}"
        result = {guid}
        for dependency in graph[guid]:
            result |= closure(dependency, ancestors + (guid,))
        return result

    closures = {name: closure(guid) for name, guid in ids.items()}
    assert set(graph[ids["core"]]) == set("58D0FB3206B6F859 65AD7BE594439A52 5B0D1E4380971EBD".split())
    assert {"65AD7D0D9941A380", "6586079789278413"} <= closures["core"], "ACE Core/Medical Core Dev missing"
    ace_modules = set(graph["65AD7BE594439A52"]) - vanilla
    assert not (ace_modules & set(graph[ids["units"]])), "Individual ACE dependencies duplicated in Units"
    assert graph[ids["lobby"]] == ["58D0FB3206B6F859"]
    for name in ("core", "units", "missions", "lobby"):
        assert ids["workshop"] not in closures[name], f"Collection leaked into {name}"
    for guid in (ids["units"], ids["missions"], ids["lobby"], "65AD60E204191D37", "61F0E6AB2D06B1C6"):
        assert guid not in closures["core"], f"Unwanted Core requirement: {guid}"
    for guid in ("6318DCB19B389CC8", "684CE8AA3B1D6573"):
        assert guid in graph[ids["units"]], f"Missing explicit gameplay baseline: {guid}"
    previous = set(json.loads((here / "previous-core-dependencies.json").read_text()))
    assert previous <= closures["workshop"], "Collection lost a previous selection"
    assert all(ids[name] in closures["workshop"] for name in ("core", "units", "missions", "lobby"))
    core_root = local["core"].parent
    for folder in ("Prefabs/Weapons", "Prefabs/Items/Equipment", "Prefabs/Characters/Factions", "worlds"):
        assert not any(path.is_file() for path in (core_root / folder).rglob("*")), f"Content left in Core: {folder}"
    assert not (core_root / "Prefabs/Vehicles/Core/Vehicle_Base.et").exists()
    moves = json.loads((here / "moves.json").read_text())
    for move in moves:
        source, target = root / move["from"], root / move["to"]
        assert source.resolve().is_relative_to(root) and target.resolve().is_relative_to(root)
        assert not source.exists(), f"Duplicate source remains: {source}"
        digest = hashlib.sha256(target.read_bytes()).hexdigest()
        assert digest == move.get("mergedSha256", move["sha256"]), f"Moved resource changed: {target}"
    print(json.dumps({"result": "PASS", "movedFiles": len(moves), "addonsIncludingSelfAndBase": {name: len(values) for name, values in closures.items()}}, indent=2))


if __name__ == "__main__":
    main()
