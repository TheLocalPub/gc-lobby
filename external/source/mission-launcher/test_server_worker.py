import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import MagicMock, patch

from server_worker import Api, Worker, receipt_matches


class WorkerChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.settings = {"stateDirectory": self.temp.name, "deployments": {}, "projects": []}

    def test_receipt_requires_nonce_world_and_exact_mods(self):
        plan = {"scenario": "{1111111111111111}Mission.conf", "world": "{2222222222222222}World.ent",
                "mods": [{"modId": "3333333333333333"}], "worldFile": "$Mission:World.ent"}
        receipt = {"version": 1, "launchId": "nonce", "scenario": plan["scenario"], "world": plan["world"],
                   "worldFile": "$Mission:World.ent", "headerWorldFile": "$Mission:World.ent",
                   "addonGuids": ["3333333333333333", "58D0FB3206B6F859"]}
        self.assertTrue(receipt_matches(receipt, plan, "nonce"))
        self.assertFalse(receipt_matches(receipt, plan, "different"))
        for field, value in [("scenario", "{AAAAAAAAAAAAAAAA}Wrong.conf"), ("world", "{AAAAAAAAAAAAAAAA}Wrong.ent"),
                             ("worldFile", "$Wrong:World.ent"),
                             ("addonGuids", []), ("addonGuids", ["3333333333333333", "AAAAAAAAAAAAAAAA"])]:
            changed = copy.deepcopy(receipt)
            changed[field] = value
            with self.assertRaises(ValueError):
                receipt_matches(changed, plan, "nonce")

    def test_uncertain_server_requires_reconciliation(self):
        for phase in ["starting", "serving"]:
            (Path(self.temp.name) / "state.json").write_text(json.dumps({"phase": phase, "processId": 123}))
            with self.assertRaisesRegex(ValueError, "ownership is uncertain"):
                Worker(self.settings, None)

    def test_claim_survives_lost_http_response(self):
        class LostResponse:
            def post(self, path, value):
                raise TimeoutError()
        first = Worker(self.settings, LostResponse())
        with self.assertRaises(TimeoutError):
            first.step()
        second = Worker(self.settings, LostResponse())
        self.assertEqual(first.state["claimId"], second.state["claimId"])

    def test_terminal_claim_is_never_launched(self):
        class Terminal:
            def post(self, path, value):
                return {"claimId": value["claimId"], "status": "completed"}
        worker = Worker(self.settings, Terminal())
        with patch("server_worker.subprocess.Popen") as launch:
            worker.step()
            launch.assert_not_called()
        self.assertEqual(worker.state, {"phase": "idle"})

    def test_missing_deployment_reports_failure_without_starting(self):
        class Assignment:
            def post(self, path, value):
                return {"claimId": value["claimId"], "status": "running", "missionId": "missing", "requestId": "request"}
        worker = Worker(self.settings, Assignment())
        with patch("server_worker.subprocess.Popen") as launch:
            worker.step()
            launch.assert_not_called()
        self.assertEqual(worker.state["result"]["status"], "failed")

    def test_remote_plaintext_and_embedded_credentials_rejected(self):
        for url in ["http://server.example", "https://user:secret@server.example", "https://server.example/?secret=x"]:
            with self.assertRaises(ValueError):
                Api(url, "key")

    def test_child_success_timeout_and_mismatch(self):
        for outcome in ["success", "timeout", "wrong-world"]:
            with self.subTest(outcome=outcome), tempfile.TemporaryDirectory() as directory:
                assignment = {"missionId": "mission", "requestId": "request", "status": "running"}
                api = MagicMock()
                api.post.side_effect = lambda path, value: {**assignment, "claimId": value["claimId"]}
                settings = {"stateDirectory": directory, "deployments": {"mission": {}}, "projects": [],
                            "coreAddonIds": [], "serverTemplate": {}, "mode": "local", "executable": "fake.exe",
                            "startupTimeoutSeconds": 0 if outcome == "timeout" else 5}
                plan = {"scenario": "{1111111111111111}Mission.conf", "world": "{2222222222222222}World.ent",
                        "worldFile": "$Mission:World.ent", "mods": [], "planSha256": "a" * 64}
                def prepare(plan, executable, run, nonce, base):
                    profile = run / "profile"
                    (profile / "profile").mkdir(parents=True)
                    (profile / "profile/GCLaunchReceipt.json").write_text(json.dumps({
                        "version": 1, "launchId": nonce, "scenario": plan["scenario"], "world": plan["world"],
                        "worldFile": "$Wrong:World.ent" if outcome == "wrong-world" else plan["worldFile"], "addonGuids": []}))
                    return [str(Path(directory) / "fake.exe")], profile
                child = MagicMock(pid=123)
                child.poll.return_value = None
                worker = Worker(settings, api)
                with patch("server_worker.make_plan", return_value=plan), patch("server_worker.projects", return_value={}), \
                        patch("server_worker.arguments", side_effect=prepare), patch("server_worker.subprocess.Popen", return_value=child):
                    worker.step()
                self.assertEqual(worker.state["result"]["status"], "completed" if outcome == "success" else "failed")
                self.assertEqual(child.terminate.called, outcome != "success")
                if outcome == "success":
                    # A failed outcome HTTP delivery retains the exact outcome and child.
                    previous = copy.deepcopy(worker.state["result"])
                    api.post.side_effect = TimeoutError()
                    with self.assertRaises(TimeoutError):
                        worker.step()
                    self.assertEqual(worker.state["result"], previous)
                    self.assertIs(worker.child, child)
                worker.stop_child()

    def test_planning_failure_preserves_existing_child(self):
        api = MagicMock()
        api.post.side_effect = lambda path, value: {"claimId": value["claimId"], "status": "running", "missionId": "missing", "requestId": "request"}
        worker = Worker(self.settings, api)
        child = MagicMock()
        child.poll.return_value = None
        worker.child = child
        worker.state["phase"] = "serving"
        worker.step()
        child.terminate.assert_not_called()
        self.assertIs(worker.child, child)


if __name__ == "__main__":
    unittest.main()
