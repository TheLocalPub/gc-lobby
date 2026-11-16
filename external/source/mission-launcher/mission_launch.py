"""Experimental saved-source dependency scanner. Python 3.12, standard library only.

Never treats an unreadable resource, packed resource, or script as a leaf.
The resulting mission sidecar is deliberately not launchable when coverage is unknown.
"""
from __future__ import annotations

import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

VANILLA = {"5614BBCCBB55ED1C", "58D0FB3206B6F859"}
TEXT = {".conf", ".ent", ".layer", ".et", ".emat", ".gamemat", ".layout",
        ".ptc", ".acp", ".imageset", ".ct", ".agf", ".agr", ".ast", ".pap"}
RESOURCE_EXTENSIONS = TEXT | {".xob", ".edds", ".wav", ".ogg", ".png", ".tga", ".smap", ".nmn", ".terr", ".anm"}
# No binary extension is assumed to contain no references.
REFERENCE = re.compile(r"\{([0-9A-Fa-f]{16})\}([^\s\"{}]+\.[A-Za-z0-9_]+)")
TOKEN = re.compile(r'\s+|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+', re.S)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tokens(text):
    if "\0" in text:
        raise ValueError("Binary data cannot be scanned as native text")
    result = []
    end = 0
    for match in TOKEN.finditer(text):
        if match.start() != end:
            raise ValueError("Unsupported or unterminated native text")
        end = match.end()
        value = match.group()
        if value.startswith("/*") and not value.endswith("*/"):
            raise ValueError("Unterminated native comment")
        if not value.isspace() and not value.startswith(("//", "/*")):
            result.append(value)
    if end != len(text):
        raise ValueError("Unterminated native text")
    return result


def unquote(value):
    if not value.startswith('"') or not value.endswith('"'):
        raise ValueError("Expected a quoted native value")
    return re.sub(r'\\(["\\])', r'\1', value[1:-1])


def block(values, start):
    if values[start] != "{":
        raise ValueError("Expected native block")
    depth = 1
    for end in range(start + 1, len(values)):
        depth += (values[end] == "{") - (values[end] == "}")
        if depth == 0:
            return values[start + 1:end], end + 1
    raise ValueError("Unterminated native block")


def root(path):
    values = tokens(Path(path).read_text(encoding="utf-8-sig"))
    start = values.index("{")
    body, end = block(values, start)
    if end != len(values):
        raise ValueError(f"Multiple root containers: {path}")
    return values[:start], body


def field(values, name, array=False):
    depth = 0
    found = []
    for i, value in enumerate(values):
        if depth == 0 and value == name:
            if array:
                opening = i + 1
                while opening < len(values) and values[opening] != "{":
                    opening += 1
                found.append(block(values, opening)[0])
            else:
                found.append(unquote(values[i + 1]))
        depth += (value == "{") - (value == "}")
    if depth or len(found) > 1:
        raise ValueError(f"Malformed or duplicate field: {name}")
    return found[0] if found else ([] if array else "")


def guid(value):
    if not re.fullmatch(r"[0-9a-fA-F]{16}", value):
        raise ValueError(f"Invalid addon GUID: {value!r}")
    return value.upper()


def reference(value):
    match = REFERENCE.fullmatch(value)
    if not match:
        raise ValueError(f"Expected GUID-qualified resource: {value}")
    path = match[2].replace("\\", "/")
    if path.startswith("/") or ":" in path or ".." in path.split("/"):
        raise ValueError(f"Invalid resource path: {value}")
    return match[1].upper(), path


class Scanner:
    def __init__(self, projects):
        self.projects = {}
        self.resources = {}
        self.inputs = {}
        self.issues = set()
        self.edges = []
        self.required = {}
        self.layer_sets = {}
        self.script_sets = {}
        self.meta_sets = {}
        self.archive_sets = {}
        self.warnings = set()
        self.pending = deque()
        for path in sorted({Path(p).resolve() for p in projects}):
            heading, body = root(path)
            if heading != ["GameProject"]:
                raise ValueError(f"Unsupported project: {path}")
            addon = guid(field(body, "GUID"))
            if addon in self.projects:
                raise ValueError(f"Duplicate addon identity: {addon}")
            self.projects[addon] = {"path": path, "id": field(body, "ID"),
                "name": field(body, "TITLE"),
                "dependencies": [guid(unquote(v)) for v in field(body, "Dependencies", True)]}
        # Index metadata only. Do not recursively read unrelated assets.
        for addon, project in self.projects.items():
            metadata = source_metadata(project["path"].parent)
            self.meta_sets[str(project["path"].parent)] = [str(p) for p in metadata]
            for meta in metadata:
                self.track(meta)
                try:
                    _, body = root(meta)
                    resource = field(body, "Name")
                    resource_id, relative = reference(resource)
                    source = meta.with_suffix("")
                    source.resolve().relative_to(project["path"].parent)
                    actual = source.relative_to(project["path"].parent).as_posix()
                    self.resources.setdefault(resource_id, []).append((addon, source, meta, actual))
                except (ValueError, UnicodeError, IndexError):
                    # Unusable metadata cannot resolve a reference; it is never guessed by path.
                    continue

    def track(self, path):
        path = Path(path).resolve()
        self.inputs[str(path)] = digest(path)

    def references(self, values, origin, queue):
        for token in values:
            if not token.startswith('"'):
                continue
            text = unquote(token)
            matches = list(REFERENCE.finditer(text))
            for match in matches:
                queue.append((match.group(), origin, None))
            # Path-only resources need the engine's mount/override resolution.
            if not matches and Path(text).suffix.lower() in RESOURCE_EXTENSIONS:
                self.issues.add(f"Unqualified resource requires engine resolution: {text} (from {origin})")

    def require(self, addon, reason):
        pending = [(addon, reason, ())]
        while pending:
            current, why, ancestors = pending.pop()
            self.required.setdefault(current, set()).add(why)
            if current in ancestors:
                self.issues.add("Addon dependency cycle: " + " -> ".join((*ancestors, current)))
                continue
            if current in VANILLA:
                continue
            project = self.projects.get(current)
            if not project:
                self.issues.add(f"Missing addon project: {current} (required by {why})")
                continue
            if project["id"].startswith(("GlobalConflictsWorkbench", "GlobalConflictsWorkshop")):
                self.issues.add(f"Authoring collection cannot be a server dependency: {current}")
            self.track(project["path"])
            if current in self.expanded:
                continue
            self.expanded.add(current)
            _, project_body = root(project["path"])
            self.references(project_body, f"project:{current}", self.pending)
            if "Modules" in project_body:
                self.issues.add(f"Custom script modules require engine verification: {current}")
            # Scripts can introduce resources without serialized prefab references.
            scripts = project["path"].parent / "scripts"
            if not scripts.exists():
                scripts = project["path"].parent / "Scripts"
            for script in sorted(scripts.rglob("*.c")) if scripts.exists() else []:
                if any(part.lower() in {"game", "gamecode", "gamelib", "core"} for part in script.parts):
                    self.track(script)
                    self.issues.add(f"Runtime script requires engine/dynamic dependency verification: {current}")
            self.script_sets[str(scripts)] = sorted(str(p.resolve()) for p in scripts.rglob("*.c"))
            archives = sorted(str(p.resolve()) for p in project["path"].parent.glob("*.pak"))
            self.archive_sets[str(project["path"].parent)] = archives
            if archives:
                self.issues.add(f"Packed addon needs engine/resource coverage: {current}")
            for dependency in reversed(project["dependencies"]):
                pending.append((dependency, current, (*ancestors, current)))

    def scan(self, mission, core_addon_ids=()):
        self.expanded = set()
        mission = Path(mission).resolve()
        _, meta = root(str(mission) + ".meta")
        scenario = field(meta, "Name")
        _, scenario_path = reference(scenario)
        matches = [(a, p) for a, p in self.projects.items()
                   if (p["path"].parent / scenario_path).resolve() == mission]
        if len(matches) != 1:
            raise ValueError("Mission must belong to exactly one supplied project")
        owner = matches[0][0]
        heading, body = root(mission)
        if ":" in heading:
            raise ValueError("Inherited mission headers need engine resolution; not supported yet")
        world = field(body, "World")
        reference(world)
        catalog = field(body, "m_GC_Catalog", True)
        explicit = [guid(unquote(v)) for v in field(catalog, "m_aRequiredAddonIds", True)]
        self.require(owner, "mission owner")
        core_addon_ids = sorted({guid(value) for value in core_addon_ids})
        for addon in core_addon_ids:
            self.require(addon, "GC core system policy")
        for addon in explicit:
            self.require(addon, "mission declared requirement")
        queue = self.pending
        queue.append((scenario, "scenario", None))
        visited = set()
        resources = []
        while queue:
            value, origin, direct = queue.popleft()
            resource_id, relative = reference(value)
            candidates = self.resources.get(resource_id, []) if direct is None else [direct]
            if not candidates:
                self.issues.add(f"Unresolved resource: {value} (from {origin})")
                continue
            # Multiple source definitions may be overrides. Do not select an arbitrary winner.
            if len(candidates) != 1:
                self.issues.add(f"Ambiguous resource/override: {value}")
                continue
            addon, path, meta, canonical = candidates[0]
            if canonical.lower() != relative.lower():
                self.warnings.add(f"Resolved GUID with stale path hint: {value} -> {canonical}")
            self.edges.append({"from": origin, "to": value, "addonId": addon})
            self.require(addon, value)
            key = (addon, str(path))
            if key in visited:
                continue
            visited.add(key)
            if len(visited) > 100000:
                self.issues.add("Resource scan exceeded 100000 resources")
                break
            self.track(meta)
            if not path.is_file():
                self.issues.add(f"Packed or missing resource requires engine scan: {value}")
                continue
            self.track(path)
            resources.append({"resource": value, "addonId": addon, "sha256": digest(path)})
            if path.suffix.lower() not in TEXT:
                self.issues.add(f"Unsupported resource format requires engine scan: {value}")
                continue
            try:
                values = tokens(path.read_text(encoding="utf-8-sig"))
            except (UnicodeError, ValueError):
                self.issues.add(f"Unreadable text resource: {value}")
                continue
            # Skip generated catalogue evidence: it contains unrelated loaded addons/assets.
            if path == mission:
                values = [f'"{world}"']
                # Other header references (briefing, preview, etc.) still count.
                depth = 0
                skip_until = -1
                for i, token in enumerate(body):
                    if i < skip_until:
                        continue
                    if token in {"m_GC_GeneratedMetadata", "m_GC_Catalog"} and depth == 0:
                        opening = body.index("{", i)
                        _, skip_until = block(body, opening)
                        continue
                    values.append(token)
                    depth += (token == "{") - (token == "}")
            self.references(values, value, queue)
            # Layers are an implicit world dependency, not always named in the .ent.
            if path.suffix.lower() == ".ent":
                layer_dir = path.with_name(path.stem + "_Layers")
                layers = sorted(layer_dir.rglob("*.layer")) if layer_dir.exists() else []
                self.layer_sets[str(layer_dir)] = [str(p.resolve()) for p in layers]
                for layer in layers:
                    # Layer files usually have no .meta of their own.
                    self.track(layer)
                    try:
                        layer_values = tokens(layer.read_text(encoding="utf-8-sig"))
                    except (UnicodeError, ValueError):
                        self.issues.add(f"Unreadable world layer: {layer}")
                        continue
                    self.edges.append({"from": value, "to": str(layer), "addonId": addon})
                    self.references(layer_values, str(layer), queue)
        addons = []
        for addon, reasons in sorted(self.required.items()):
            if addon in VANILLA:
                continue
            project = self.projects.get(addon)
            version = ""
            if project:
                server_data = project["path"].with_name("ServerData.json")
                if server_data.exists():
                    self.track(server_data)
                    data = json.loads(server_data.read_text(encoding="utf-8-sig"))
                    if guid(data["id"]) != addon:
                        raise ValueError(f"ServerData identity mismatch: {server_data}")
                    version = data["revision"]["version"]
            addons.append({"modId": addon, "name": project["name"] if project else addon,
                           "version": version, "required": True, "reasons": sorted(reasons)})
        return {"schemaVersion": 1, "scanner": "gc-saved-source-prototype",
                "scenarioId": scenario, "world": world, "ownerAddonId": owner,
                "coreAddonIds": core_addon_ids,
                "status": "incomplete" if self.issues else "static-complete",
                "coverage": "Saved text resource references and native addon dependency closure; not an engine/runtime certificate.",
                "issues": sorted(self.issues), "mods": addons, "resources": resources,
                "edges": self.edges, "inputs": self.inputs, "layerSets": self.layer_sets,
                "scriptSets": self.script_sets, "metadataSets": self.meta_sets,
                "archiveSets": self.archive_sets,
                "warnings": sorted(self.warnings)}


def source_metadata(directory):
    return sorted(p.resolve() for p in Path(directory).rglob("*.meta")
                  if not any(part in {".git", "external"} for part in p.relative_to(directory).parts))


def write_new(path, data):
    with Path(path).open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(data, stream, indent=2)
        stream.write("\n")


def write_collection(projects, addon_guid, output):
    """An independent, metadata-only authoring collection; never edits Core."""
    dependencies = set()
    for path in projects:
        heading, body = root(path)
        if heading != ["GameProject"]:
            raise ValueError(f"Unsupported project: {path}")
        dependencies.add(guid(field(body, "GUID")))
        dependencies.update(guid(unquote(v)) for v in field(body, "Dependencies", True))
    addon_guid = guid(addon_guid)
    if addon_guid in dependencies or addon_guid in VANILLA:
        raise ValueError("Collection must have its own new addon identity")
    lines = ['GameProject {', ' ID "GlobalConflictsWorkshopDev"',
             f' GUID "{addon_guid}"', ' TITLE "GC Workshop"',
             ' Dependencies {']
    lines.extend(f'  "{value}"' for value in sorted(dependencies))
    lines.extend([' }', ' Configurations {', '  GameProjectConfig PC {', '  }', ' }', '}'])
    with Path(output).open("x", encoding="utf-8", newline="\n") as stream:
        stream.write("\n".join(lines) + "\n")
    return len(dependencies)


def make_config(manifest, template):
    if manifest.get("schemaVersion") != 1 or manifest.get("status") != "static-complete" or manifest.get("issues"):
        raise ValueError("Dependency scan is incomplete; server configuration was not generated")
    for path, expected in manifest["inputs"].items():
        if not Path(path).is_file() or digest(path) != expected:
            raise ValueError(f"Stale mission metadata; rescan: {path}")
    for sets, pattern in ((manifest["layerSets"], "*.layer"), (manifest["scriptSets"], "*.c")):
        for directory, expected in sets.items():
            actual = sorted(str(p.resolve()) for p in Path(directory).rglob(pattern))
            if actual != expected:
                raise ValueError(f"Source file set changed; rescan: {directory}")
    for directory, expected in manifest["metadataSets"].items():
        if [str(p) for p in source_metadata(directory)] != expected:
            raise ValueError(f"Resource metadata set changed; rescan: {directory}")
    for directory, expected in manifest["archiveSets"].items():
        actual = sorted(str(p.resolve()) for p in Path(directory).glob("*.pak"))
        if actual != expected:
            raise ValueError(f"Packed archive set changed; rescan: {directory}")
    config = json.loads(json.dumps(template))
    game = config["game"]
    game["scenarioId"] = manifest["scenarioId"]
    game["mods"] = [{k: mod[k] for k in ("modId", "name", "version", "required")}
                    for mod in manifest["mods"]]
    if any(not mod["version"] for mod in game["mods"]):
        raise ValueError("Workshop configuration requires installed version metadata for every selected addon")
    game["modsRequiredByDefault"] = True
    return config


def launch(config, executable, profile, output):
    """Start a stopped test instance. Never kills or changes an existing server."""
    executable = Path(executable).resolve(strict=True)
    profile = Path(profile).resolve()
    output = Path(output).resolve()
    # A new profile isolates experiments from a running/production instance.
    profile.mkdir(parents=True, exist_ok=False)
    write_new(output, config)
    with (profile / "launcher.log").open("xb") as log:
        process = subprocess.Popen([str(executable), "-config", str(output),
                                    "-profile", str(profile), "-maxFPS", "30"],
                                   cwd=executable.parent, stdout=log, stderr=subprocess.STDOUT,
                                   creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0)
        print(f"Started test server PID {process.pid}; verify scenario readiness in its logs", flush=True)
        try:
            return process.wait()
        except KeyboardInterrupt:
            process.terminate()
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            return 130


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    scan = commands.add_parser("scan")
    scan.add_argument("--project", action="append", required=True, type=Path)
    scan.add_argument("--mission", required=True, type=Path)
    scan.add_argument("--output", required=True, type=Path)
    scan.add_argument("--core-mod", action="append", default=[],
                      help="Always-required system addon GUID; repeat for the selected GC core policy")
    configure = commands.add_parser("config")
    configure.add_argument("--metadata", required=True, type=Path)
    configure.add_argument("--template", required=True, type=Path)
    configure.add_argument("--output", required=True, type=Path)
    start = commands.add_parser("launch")
    start.add_argument("--metadata", required=True, type=Path)
    start.add_argument("--template", required=True, type=Path)
    start.add_argument("--output", required=True, type=Path)
    start.add_argument("--executable", required=True, type=Path)
    start.add_argument("--profile", required=True, type=Path)
    collection = commands.add_parser("collection")
    collection.add_argument("--project", action="append", required=True, type=Path)
    collection.add_argument("--guid", required=True)
    collection.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.command == "collection":
        count = write_collection(args.project, args.guid, args.output)
        print(f"Created GC Workshop with {count} direct dependencies")
        return 0
    if args.command == "scan":
        scanner = Scanner(args.project)
        result = scanner.scan(args.mission, args.core_mod)
        write_new(args.output, result)
        print(f'{result["status"]}: {len(result["mods"])} mods, {len(result["resources"])} resources, {len(result["issues"])} unresolved checks')
        return 2 if result["issues"] else 0
    manifest = json.loads(args.metadata.read_text(encoding="utf-8-sig"))
    template = json.loads(args.template.read_text(encoding="utf-8-sig"))
    config = make_config(manifest, template)
    if args.command == "launch":
        return launch(config, args.executable, args.profile, args.output)
    write_new(args.output, config)
    print("Created server configuration")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError, IndexError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
