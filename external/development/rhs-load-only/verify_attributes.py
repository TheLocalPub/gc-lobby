"""Verify this explicit experiment's saved report and three process logs.

This checks fixture evidence; it is not a production dependency selector.
"""
import argparse
from collections import Counter
import json
import re
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / "source" / "mission-launcher"))
from mission_launch import block, digest, field, root, unquote


def objects(values):
    index = 0
    while index < len(values):
        opening = values.index("{", index)
        body, index = block(values, opening)
        yield body


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--clients", type=Path, nargs=2, required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = HERE / "attribute-evidence.conf"
    _, data = root(report)
    require(not field(data, "m_sError"), "Collector exceeded a limit or failed")
    rows = [dict(origin=field(row, "m_sOrigin"), resource=field(row, "m_sResource"),
                 status=field(row, "m_sStatus"),
                 addons=[unquote(x) for x in field(row, "m_aAddonIds", True)])
            for row in objects(field(data, "m_aReferences", True))]
    expected = {
        "AttributeCompass.et": ("PrefabsToSpawn[0]", "GCAttributeContentTest"),
        "AttributeBandage.et": ("PrefabsToSpawn[1]", "GCAttributeContentTest"),
        "AttributeHead.et": ("VisualIdentity.Head", "GCAttributeContentTest"),
        "Rifle_M4A1_BLOCK_0.et": ("CharacterWeaponSlotComponent.WeaponTemplate", "RHS_Core"),
    }
    targets = []
    for suffix, (origin_suffix, addon) in expected.items():
        matches = [r for r in rows if r["resource"].endswith(suffix)
                   and "Prefabs/AttributeCharacter.et/" in r["origin"]
                   and r["origin"].endswith(origin_suffix)]
        require(len(matches) == 1 and matches[0]["status"] == "container"
                and addon in matches[0]["addons"], "Missing attribute evidence: " + suffix)
        targets.extend(matches)
    root_row = next(r for r in rows if r["origin"] == "root")
    require("GCAttributeContentTest" not in root_row["addons"], "Fixture no longer isolates attribute-only provenance")
    addon_map = {field(a, "m_sId"): field(a, "m_sGuid")
                 for a in objects(field(data, "m_aLoadedAddons", True))}
    require(addon_map.get("GCAttributeContentTest") == "7E161126A01D1001", "Missing provider GUID")
    require(addon_map.get("RHS_Core") == "595F2BF2F44836FB", "Missing RHS GUID")
    _, project = root(HERE / "addon.gproj")
    require([unquote(x) for x in field(project, "Dependencies", True)] == ["6A115D5DFF7ED870"], "Fixture dependencies changed")
    paths = [args.server, *args.clients]
    require(len({p.resolve() for p in paths}) == 3, "Use three distinct process logs")
    logs = [p.read_text(encoding="utf-8-sig") for p in paths]
    for index, log in enumerate(logs):
        server = int(index == 0)
        prefix = f"GC_ATTRIBUTE_TEST run={args.run} server={server}"
        for suffix in list(expected)[:3]:
            require(any(prefix + " resourceValid=1" in line and line.endswith(suffix)
                        for line in log.splitlines()), f"Process {index} cannot resolve {suffix}")
        samples = re.findall(r"GC_RHS_TEST run=" + re.escape(args.run)
                             + rf" server={server} sample=(\d+) rhsEntity=\d+ players=(\d+)", log)
        require(sum(int(players) == 2 for sample, players in samples) >= 12,
                f"Process {index} lacks a minute of two-player evidence")
        require("ReplicationError:" not in log and "Kicked from game." not in log,
                f"Process {index} reports replication failure or kick")
    for player in (1, 2):
        require(f"OnPlayerAuditSuccess: {player}, connected!" in logs[0], "Missing server player audit")
    require("Players connected: 2 / 2" in logs[0], "Missing server connection result")
    for log in logs[1:]:
        require("becameOwner=1 server=0" in log, "Client has no owned controller")
    for suffix in ("AttributeCompass.et", "AttributeBandage.et", "Rifle_M4A1_BLOCK_0.et"):
        require(any(f"GC_ATTRIBUTE_TEST run={args.run} server=1 inventory=" in line
                    and line.endswith(suffix) for line in logs[0].splitlines()), "Missing actual inventory: " + suffix)
    require(any(f"GC_ATTRIBUTE_TEST run={args.run} server=1 head=" in line
                and line.endswith("AttributeHead.et") for line in logs[0].splitlines()), "Authored head was not used")
    result = dict(run=args.run, result="passed", coverage="partial_resource_field_graph",
                  report_sha256=digest(report), status_counts=dict(Counter(r["status"] for r in rows)),
                  root_provenance=root_row, attribute_references=targets,
                  addon_guids=addon_map,
                  logs=[dict(path=str(p.resolve()), sha256=digest(p)) for p in paths])
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("PASS saved attribute ownership, GUID mapping, actual inventory/head, dedicated server and two clients")


if __name__ == "__main__":
    main()
