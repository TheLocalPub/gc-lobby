"""Verify saved metadata-driven queue/server/client evidence; no game mutation."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'source/mission-launcher'))
from mission_launch import VANILLA, field, root, unquote
from server_worker import receipt_matches
from server_plan import canonical


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run-directory', type=Path, required=True)
    parser.add_argument('--outcomes', type=Path, required=True)
    parser.add_argument('--clients', nargs=2, type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--prefix-evidence', type=Path, help='Recheck a captured prefix after deliberate shutdown appended to logs')
    args = parser.parse_args()
    prefixes = json.loads(args.prefix_evidence.read_text())['logs'] if args.prefix_evidence else []
    def log_bytes(path):
        data = path.read_bytes()
        if prefixes:
            item = next(x for x in prefixes if Path(x['path']).resolve() == path.resolve())
            data = data[:item['bytes']]
            assert hashlib.sha256(data).hexdigest() == item['sha256']
        return data
    plan = json.loads((args.run_directory / 'plan.json').read_text())
    assert hashlib.sha256(canonical({k: v for k, v in plan.items() if k != 'planSha256'})).hexdigest() == plan['planSha256']
    assert args.clients[0].resolve() != args.clients[1].resolve()
    receipt = json.loads((args.run_directory / 'profile/profile/GCLaunchReceipt.json').read_text())
    server = next((args.run_directory / 'profile/logs').glob('*/console.log'))
    server_text = log_bytes(server).decode(errors='replace')
    assert f"-gcLaunchId {receipt['launchId']}" in server_text
    assert receipt_matches(receipt, plan, receipt['launchId'])
    outcomes = json.loads(args.outcomes.read_text(encoding='utf-8-sig'))
    outcome = next(x for x in outcomes if x['request_id'] == plan['requestId'])
    assert outcome['status'] == 'completed'
    assert outcome['outcome']['planSha256'] == plan['planSha256']
    selected = {m['modId'] for m in plan['mods']}
    assert {'623555110E2B2CA1', '595F2BF2F44836FB', '7E161126A01D1001'} <= selected
    _, project = root(Path(__file__).with_name('addon.gproj'))
    declared = [unquote(x) for x in field(project, 'Dependencies', True)]
    assert '595F2BF2F44836FB' not in declared and '7E161126A01D1001' not in declared
    assert 'GC | OnPlayerAuditSuccess: 1, connected!' in server_text
    assert 'GC | OnPlayerAuditSuccess: 2, connected!' in server_text
    forbidden = ['script protocol checksum does not match', 'Server event Disconnected', 'Client event Disconnected', 'REPLICATION/SYSTEM_FAILURE']
    logs = []
    for path in [server, *args.clients]:
        data = log_bytes(path)
        text = data.decode(errors='replace')
        assert not any(x in text for x in forbidden), str(path)
        if path != server:
            assert 'becameOwner=1 server=0' in text
            samples = [int(x) for x in re.findall(r'GC_RHS_TEST run=queue-launch-01 server=0 sample=(\d+) rhsEntity=\d players=2', text)]
            assert samples == list(range(1, 37)), (str(path), samples)
        logs.append({'path': str(path), 'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)})
    evidence = {'result': 'passed', 'scope': 'installed local addons, release dedicated server and two release clients launched by the dedicated tool',
                'requestId': plan['requestId'], 'claimId': outcome['claim_id'], 'processId': outcome['outcome']['processId'],
                'planSha256': plan['planSha256'], 'scenario': receipt['scenario'], 'actualWorld': receipt['worldFile'],
                'selectedNonVanillaAddons': len(selected), 'loadedNonVanillaAddons': len(set(receipt['addonGuids']) - VANILLA),
                'declaredMissionDependencies': declared, 'consecutiveTwoPlayerSamplesPerClient': 36,
                'configuredSampleCadenceSeconds': 5, 'logs': logs,
                'limits': ['Workshop downloads and remote joining not tested',
                           'Dedicated tool clients retained native timeout-disable and reconnect flags; network timeout handling not tested',
                           'Client-side named weapon/character lookup is outside streaming range; this test proves connection, not client equipment visibility',
                           'Diagnostic client/server builds with Core failed script-checksum validation; release pair passed']}
    args.output.write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
