"""Verify production native metadata contains nested and world-instance references."""
from collections import Counter
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / "source" / "mission-launcher"))
from mission_launch import digest, field as native_field, root, unquote
from verify_attributes import objects, require

def field(values, name, array=False):
    if array:
        return native_field(values, name, True)
    depth = 0
    for index, value in enumerate(values):
        if depth == 0 and value == name:
            result = values[index + 1]
            return unquote(result) if result.startswith('"') else result
        depth += (value == "{") - (value == "}")
    return ""

header = HERE / "Configs/Missions/RhsLoadOnly.conf"
_, data = root(header)
metadata = field(data, "m_GC_GeneratedMetadata", True)
require(field(metadata, "m_iSchemaVersion") == "2", "Expected schema 2")
require(field(metadata, "m_sResourceCoverage") == "authored_container_graph_partial", "Missing coverage contract")
rows = [dict(resource=field(r, "m_sResource"), origin=field(r, "m_sOrigin"), status=field(r, "m_sStatus"),
             addons=[unquote(x) for x in field(r, "m_aAddonIds", True)])
        for r in objects(field(metadata, "m_aResourceReferences", True))]
require(len(rows) == len({r["resource"] for r in rows}), "Duplicate resource evidence")
expected = {"AttributeCompass.et": "PrefabsToSpawn[0]", "AttributeBandage.et": "PrefabsToSpawn[1]",
            "AttributeInstanceCompass.et": "PrefabsToSpawn[2]", "AttributeHead.et": "VisualIdentity.Head"}
targets = []
for suffix, origin in expected.items():
    found = [r for r in rows if r["resource"].endswith(suffix)]
    require(len(found) == 1 and found[0]["status"] == "container" and "GCAttributeContentTest" in found[0]["addons"], suffix)
    require(found[0]["origin"].startswith("entity:GC_AttributeCharacter/") and found[0]["origin"].endswith(origin), "Wrong effective field: " + suffix)
    targets += found
require("AttributeInstanceCompass" not in (HERE / "Prefabs/AttributeCharacter.et").read_text(), "Instance fixture leaked into prefab")
referenced = [unquote(x) for x in field(metadata, "m_aReferencedAddonIds", True)]
require("GCAttributeContentTest" in referenced and "RHS_Core" in referenced, "Missing nested owners")
addons = {field(a, "m_sId"): a for a in objects(field(metadata, "m_aLoadedAddons", True))}
provider = addons["GCAttributeContentTest"]
require(field(provider, "m_sGuid") == "7E161126A01D1001", "Missing GUID")
require(field(provider, "m_bReferencedByPlacedPrefab") == "0" and field(provider, "m_bReferencedByResource") == "1", "Changed placed-prefab meaning")
require(field(addons["RHS_Core"], "m_sGuid") == "595F2BF2F44836FB", "Missing RHS GUID")
require(not any("m_GC_GeneratedMetadata" in r["origin"] for r in rows), "Generated evidence scanned as input")
result = dict(result="passed", schema=2, header_sha256=digest(header), references=len(rows),
              status_counts=dict(Counter(r["status"] for r in rows)),
              unexpanded_resources=int(field(metadata, "m_iUnexpandedResourceCount")),
              omitted_vanilla_resources=int(field(metadata, "m_iOmittedVanillaResourceCount")), referenced_addons=referenced,
              provider_guid=field(provider,"m_sGuid"), target_references=targets)
(HERE / "evidence/nested-metadata.json").write_text(json.dumps(result, indent=2)+"\n")
print("PASS production metadata: inherited fields, world-instance override, deduplication, addon flags and GUIDs")
