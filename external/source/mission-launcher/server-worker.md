# Dedicated mission worker

`server_worker.py --settings <private-settings.json>` claims requests through the
website's server-scoped machine API. Python 3.12+, the game installation, the
selected addons and a trusted deployment registry are required. Workbench and
Reforger MCP are not runtime dependencies.

Settings fields:

| Field | Meaning |
| --- | --- |
| `apiUrl` | Website origin; HTTPS, except loopback development |
| `credentialEnvironment` | Name of the environment variable containing this server's Ingest key |
| `stateDirectory` | Private directory for the lock, durable claim, plans and separate launch profiles |
| `executable` | Dedicated server executable |
| `projects` | Explicit paths to installed `.gproj` files |
| `coreAddonIds` | Required baseline addon GUIDs, including GC Core |
| `deployments` | Mission-ID-keyed [deployment records](server-plan.md) |
| `mode` | `local` or `workshop` |
| `serverTemplate` | Standard server JSON settings; applied only in Workshop mode |
| `startupTimeoutSeconds` | Optional acknowledgement deadline, default 180 |
| `pollSeconds` | Optional queue interval, default 3 |
| `localBaseProject` | Optional base-game `.gproj` for a local development installation |

The worker writes a launch plan, starts a child without a shell, and waits for
`<launch profile>/profile/GCLaunchReceipt.json`. GC Lobby's own game mode writes
this only on the dedicated authority after initialization, when explicitly
started with a launch nonce. Verification requires the matching nonce, scenario
GUID, header world GUID, actual addon-qualified world path and exact non-vanilla
addon GUID set. No global game-mode hook or editor automation is installed.
This acknowledges mission initialization; it does not claim that all dynamic
gameplay resources or future player connections have been tested.

## Local development versus Workshop

The tested engine rejects `-config` combined with `-addons`. Local mode therefore
uses `-server <scenario.conf> -addonsDir <isolated directory> -addons <selected
IDs>`. This loads the scenario header, rather than launching its world directly.
It uses native local RPL port **2001**, with the engine's default listening
address. **JSON names, passwords, capacity, visibility and other server settings
are not applied in local mode.** Use an isolated development environment. A
non-default `bindPort` is rejected to avoid implying that it was applied.

Workshop mode uses `-config`, with the requested scenario and version-pinned
`game.mods`. Every selected addon needs installed version metadata. This path
has not yet been verified against published GC Dev/test packages; local tests
do not prove Workshop publishing, downloading or remote joining.

## Ownership and recovery

One OS file lock permits one worker per state directory. Persisted claim IDs
make retries return the same assignment. Planning happens before the existing
owned child is stopped, so invalid requests preserve the running server. Failed
new startups are stopped and reported failed. Result delivery retries retain
the outcome rather than launching again. The worker never kills a saved PID;
it uses only its live child-process handle.

Create `stop.txt` in the state directory for a controlled stop, then wait for the
worker to exit. Remove that file before restarting. Plans, logs and profiles are
retained for diagnosis and can contain private server configuration.

After a crash with state `starting` or `serving`, startup refuses automatic
relaunch. An operator must establish what happened to that worker's child,
stop/reconcile it as appropriate, and reconcile the corresponding website claim
before archiving the state. Running claims never expire automatically. Do not
delete state merely because an HTTP response or observation timed out. Automated
crash adoption is not implemented.

Tests: `python -m unittest -v test_server_worker test_server_plan
test_mission_launch`. Coverage includes stale deployments, exact addon closure,
receipt identity mismatches, child success/timeout cleanup, lost HTTP responses,
terminal claim retries and preserving an existing child on planning failure.
