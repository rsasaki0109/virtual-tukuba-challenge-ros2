# virtual_tukuba_challenge_v2_ws plan

## 1. Project summary

This workspace exists to make Virtual Tsukuba Challenge (VTC) usable from ROS2
without first requiring a full rewrite of the simulator itself.

The practical goal is not "port the entire simulator to ROS2."
The practical goal is:

- run the existing VTC simulator as-is, or with minimal simulator-side changes
- control it from ROS2
- receive robot state and sensor data in ROS2
- record rosbag2 data
- run LiDAR SLAM and related mapping workflows on top of that data
- eventually turn the resulting workflow into a reproducible virtual challenge
  or benchmark environment

This project should be treated as a bridge-and-workflow project first, and only
secondarily as a simulator-porting project.

That distinction matters. A full ROS2-native simulator port would be expensive,
slow, and risky. A ROS2 bridge plus a documented workflow is much cheaper, gets
to user value faster, and is enough to create a useful virtual mapping
competition or benchmark.

## 2. Why this project exists

The motivation is straightforward:

- physical field events such as Tsukuba Challenge are valuable but expensive
- public open datasets are useful but static
- a reusable virtual environment makes repeated evaluation much cheaper
- a ROS2-facing VTC workflow would make it easier to validate LiDAR SLAM,
  localization, and mapping behavior under controlled conditions

This project aims to sit between public datasets and real-world field
deployment:

- lower cost than physical participation
- more interactive than offline datasets
- more reproducible than ad hoc field experiments

If successful, the workspace should support:

- SLAM regression testing
- benchmark creation
- replayable challenge tasks
- optional future competition packaging

## 3. High-level product definition

The intended deliverable is a ROS2-compatible VTC integration stack.

At minimum, that means:

- a ROS2 bridge for robot command and state
- a ROS2-compatible sensor pipeline
- rosbag2-friendly launch workflows
- a documented procedure to generate maps inside VTC
- a reproducible evaluation flow

At maturity, that means:

- a small set of standardized scenarios
- a standard bag/log/report artifact format
- automated validation scripts
- challenge-style scoring rules

The project should remain focused on enabling evaluation and development.
It should not drift into trying to become a full simulator platform fork unless
there is overwhelming evidence that the bridge approach is insufficient.

## 4. Guiding principles

### 4.1 Preserve the existing simulator where possible

The cheapest and most robust path is to avoid modifying VTC itself unless
strictly necessary. The initial integration should assume:

- VTC runs on Windows
- Unreal Engine and Visual Studio constraints stay intact
- existing communication paths are reused

Simulator changes should be avoided until a clear bottleneck appears.

### 4.2 Keep the public path non-GPL

The public default workflow should remain non-GPL.
This applies to:

- bridge code
- default sensor path
- default SLAM workflows
- benchmark and scoring scripts

Optional integrations can exist, but the default path must remain clean and
safe for public OSS adoption.

### 4.3 Design for reproducibility first

The project is only useful if users can repeat results.
Every major workflow should eventually reduce to:

1. start simulator
2. start ROS2 bridge
3. start data pipeline
4. record bag or run SLAM
5. save artifacts
6. run report script

Anything that cannot be explained and reproduced in a short set of commands
should be treated as unfinished.

### 4.4 Keep the first versions narrow

It is better to support:

- one robot
- one LiDAR path
- one state interface
- one documented scenario

than to prematurely build a large generic abstraction that never stabilizes.

## 5. Scope

### 5.1 In scope

- ROS2 bridge to VTC-compatible robot control/state APIs
- ROS2 launch workflows
- rosbag2 recording and replay workflows
- LiDAR sensor ingestion into ROS2
- optional IMU and GNSS ingestion into ROS2
- TF and odometry publication
- benchmark/report tooling
- challenge-oriented scenario workflow design

### 5.2 Explicitly out of scope for early phases

- rewriting VTC into a Linux-native simulator
- replacing Unreal Engine
- making the entire simulator ROS2-native internally
- full autonomy stack integration by default
- lanelet generation and full planning stack support
- cloud infrastructure for competition hosting
- real-time leaderboard services

### 5.3 Possible later scope

- scenario reset automation
- richer simulator introspection
- competition packet generation
- participant submission validation
- multi-robot support

## 6. Assumed external components

The plan assumes the existence or reuse of the following categories of systems:

- VTC simulator binaries or a buildable VTC environment
- a communication path comparable to CageClient
- a Windows runtime environment for the simulator
- a Linux ROS2 environment for SLAM and tooling
- optional standard ROS2 sensor drivers such as Velodyne drivers

The project should isolate these boundaries clearly so that future replacements
are possible.

## 7. Expected architecture

The first useful architecture should look like this:

- Windows machine or VM
  - runs VTC
  - exposes simulator control/state through an existing external interface
  - emits sensor data such as Velodyne-like packets where possible

- Linux ROS2 machine
  - runs a ROS2 bridge
  - publishes and subscribes to ROS2 topics
  - records rosbag2
  - runs SLAM, mapping, and evaluation tools

The bridge should become the main integration seam.

That yields this layered design:

1. simulator runtime layer
2. communication adapter layer
3. ROS2 bridge layer
4. sensor normalization layer
5. SLAM/evaluation layer

The simulator runtime layer should stay as untouched as possible.
The communication adapter layer should hide Windows-specific or proprietary API
details from the ROS2 packages.
The ROS2 bridge layer should present stable topics, services, and actions.

## 8. Repository and package layout proposal

The workspace should likely evolve toward this package structure:

- `vtc_msgs`
  - custom message, service, and action definitions
  - simulator state, reset requests, scenario info, and diagnostics

- `vtc_bridge`
  - core bridge node(s)
  - communication with simulator-side client libraries
  - command translation
  - state publication

- `vtc_sensors`
  - optional helpers for sensor adaptation
  - packet forwarding support
  - frame and metadata normalization

- `vtc_bringup`
  - launch files
  - configuration
  - standard workflows

- `vtc_tools`
  - benchmark scripts
  - log conversion
  - report generation
  - scenario helpers

- `vtc_examples`
  - minimal example launches
  - bag recording recipes
  - sample reports

This keeps interfaces, runtime, and tooling separate.

## 9. First ROS2 API proposal

The first version should expose only what is required for mapping and motion.

### 9.1 Subscriptions

- `/cmd_vel`
  - geometry-based planar velocity command

- optional direct control topics
  - for future steering/throttle/brake style robots if needed

### 9.2 Publications

- `/odom`
  - robot odometry in a stable frame

- `/tf`
  - robot pose tree

- `/joint_states`
  - if wheel or articulation state is available

- `/points_raw`
  - LiDAR data

- `/imu/data`
  - if available

- `/gnss/fix`
  - if available

- `/vtc/state`
  - simulator-specific robot state

- `/vtc/diagnostics`
  - connection status, packet rates, simulator timing

### 9.3 Services

- `/vtc/reset`
  - reset robot and scenario state

- `/vtc/pause`
  - pause simulator

- `/vtc/resume`
  - resume simulator

- `/vtc/get_scenario_info`
  - metadata about current run

### 9.4 Actions

Actions are not required initially.
If they become necessary, likely candidates are:

- scenario start/stop
- scripted route execution

## 10. Simulator communication strategy

This is one of the most important design choices.

The bridge must not directly depend on large Unreal-specific logic.
Instead, the communication path should be thin and explicitly layered.

Preferred order:

1. reuse existing external client APIs if they are available and practical
2. wrap them in a C++ adapter with a narrow interface
3. call that adapter from ROS2 nodes

The ROS2 side should never need to understand Unreal internals.

The adapter interface should look conceptually like:

- connect
- disconnect
- send_velocity_command
- receive_robot_state
- receive_clock or timestamps if available
- receive scenario metadata

This adapter boundary should be unit-testable with mock implementations.

## 11. Sensor integration strategy

### 11.1 LiDAR

The first choice should be to use the simulator's existing Velodyne-like output.
That gives the strongest reuse and keeps the simulator side simple.

Pipeline target:

- simulator emits packets
- ROS2 Velodyne driver stack ingests packets
- ROS2 point cloud is published on `/points_raw`

Benefits:

- standard ROS2 tooling
- minimal custom sensor code
- easier debugging

### 11.2 IMU

If the simulator already exposes IMU-like state, publish it in standard
`sensor_msgs/msg/Imu`.

If not available at first, IMU can be deferred.
The first milestone should not block on IMU unless LiDAR motion distortion
becomes unusable without it.

### 11.3 GNSS

If world coordinates or georeferenced coordinates can be derived, publish
`sensor_msgs/msg/NavSatFix`.

If exact GNSS fidelity is unavailable, it is still acceptable to delay GNSS
until later phases, provided the mapping workflow can start with local maps.

### 11.4 Time

Time semantics are likely to become one of the hardest parts.
The bridge should decide early whether:

- ROS time comes from simulator time
- ROS time comes from wall time
- rosbag recording uses receive time or simulator-stamped time

This must be documented clearly because it affects SLAM, replay, and benchmark
validity.

## 12. Coordinate frames

A clean frame policy is mandatory.

Initial target frames:

- `map`
- `odom`
- `base_link`
- LiDAR frame such as `velodyne` or `lidar`
- IMU frame if available

The project should decide:

- which frame is authoritative for robot motion
- whether `odom` is simulator-truth or bridge-estimated
- whether `map` is only generated by SLAM or also available from simulator data

Early versions should prefer a simple and explicit model:

- simulator state publishes `odom -> base_link`
- sensor frames are static transforms from `base_link`
- SLAM later creates `map -> odom`

## 13. Data recording and replay workflow

Every serious validation path should be bag-based.

The canonical workflow should become:

1. launch bridge and sensor pipeline
2. record rosbag2
3. replay the bag offline
4. run SLAM on replay
5. generate map and report

This provides:

- reproducibility
- sharable artifacts
- offline debugging
- challenge-style submission feasibility

The workspace should eventually ship a helper script to record a complete bag
with the correct topics and metadata.

## 14. Integration target with LiDAR SLAM

The first SLAM validation target should be the already-developed non-GPL public
path from the main mapping workspace.

That means:

- feed `/points_raw`
- optionally feed `/imu/data`
- optionally feed `/gnss/fix`
- run a known-good mapping stack
- validate that maps, trajectories, and reports are produced

The purpose of this workspace is not to invent a new SLAM stack first.
It is to provide a reproducible virtual environment in which existing SLAM can
be evaluated.

## 15. Benchmark philosophy

The eventual benchmark should not be "did the robot move."
It should be a proper mapping benchmark.

Recommended metrics:

- map generation success/failure
- trajectory coverage
- path length
- loop closure events
- map export success
- runtime
- sensor packet drop rate
- if reference exists:
  - APE
  - relative pose error
  - final pose error

Optional quality checks:

- map density
- point count after cleanup
- static structure retention
- dynamic object removal statistics

The first versions can be much simpler, but the artifact shape should already
anticipate this.

## 16. Virtual challenge concept

The long-term direction should be a "virtual challenge" rather than merely a
bridge package.

The challenge structure could eventually define:

- one or more standard scenarios
- standard start pose and goal pose
- fixed sensor suite
- allowed compute assumptions
- required artifact outputs
- scoring metrics

Possible score dimensions:

- task completion
- map quality
- localization stability
- runtime efficiency
- reproducibility

The important point is that the benchmark should be reproducible by anyone with
the same scenario package and instructions.

## 17. Development phases

### Phase A: Discovery and API inventory

Objective:
understand what is already available and what can be reused.

Tasks:

- inspect `cage_ros_stack` topic and service API
- identify command channels
- identify status and pose channels
- identify how LiDAR output is currently consumed
- identify whether simulator timestamps exist

Deliverables:

- API inventory document
- initial frame diagram
- initial communication diagram

Exit criteria:

- enough knowledge to define a minimal ROS2 package interface

### Phase B: Minimal bridge

Objective:
move the robot from ROS2 and read back state.

Tasks:

- create `vtc_msgs`
- create `vtc_bridge`
- subscribe to `/cmd_vel`
- publish a minimal state topic
- publish odometry

Deliverables:

- `vtc_bridge` package
- test launch file
- README quickstart for connect/move/stop

Exit criteria:

- robot can move from ROS2 commands
- state can be observed in ROS2

### Phase C: Sensor path

Objective:
obtain ROS2 LiDAR data suitable for mapping.

Tasks:

- define LiDAR path
- wire packet output to ROS2 driver stack
- normalize frame IDs
- validate point rate and packet rate

Deliverables:

- LiDAR launch workflow
- sensor diagnostics topic or tool
- bag recording recipe

Exit criteria:

- stable `/points_raw`
- enough quality to run a short mapping demo

### Phase D: SLAM validation

Objective:
generate a short map and repeat it.

Tasks:

- record a short bag
- replay it
- run SLAM
- save map and trajectory
- document the process

Deliverables:

- example bag recipe
- example map output
- first benchmark script

Exit criteria:

- at least one scenario can produce a reproducible map

### Phase E: Scenario and benchmark packaging

Objective:
turn the workflow into something others can evaluate.

Tasks:

- define standard scenario(s)
- define required artifacts
- define scoring scripts
- define participant instructions

Deliverables:

- benchmark rules
- report generator
- reference submission format

Exit criteria:

- an external user can reproduce the workflow with docs only

## 18. Validation plan

Validation should happen in layers.

### 18.1 Connectivity validation

- bridge connects
- simulator state is readable
- commands reach the robot

### 18.2 ROS2 interface validation

- topics publish at expected rates
- QoS settings are sane
- timestamps are monotonic and understandable

### 18.3 Sensor validation

- LiDAR packet and cloud rates match expectations
- frame IDs are correct
- point data is plausible
- if IMU exists, orientation and gyro values are plausible

### 18.4 Mapping validation

- short-run map generation works
- repeated runs do not diverge wildly
- exported map artifacts are valid

### 18.5 Benchmark validation

- same scenario can be rerun reproducibly
- report scripts work from generated artifacts
- failure cases are detectable and clear

## 19. Documentation plan

Documentation must arrive early, not as a final cleanup task.

Minimum docs set:

- `README.md`
  - what this workspace is
  - what it is not
  - prerequisites
  - quickstart

- `docs/architecture.md`
  - runtime diagram
  - frame diagram
  - communication boundaries

- `docs/workflows.md`
  - record
  - replay
  - map generation
  - benchmark

- `docs/scenarios.md`
  - standard scenarios
  - assumptions
  - evaluation conditions

- `docs/troubleshooting.md`
  - common connectivity issues
  - time sync issues
  - packet issues

## 20. Testing strategy

Testing should mix unit and workflow tests.

### 20.1 Unit tests

- message conversion
- adapter parsing
- frame transform helpers
- score/report generators

### 20.2 Integration tests

- bridge launch succeeds
- simulated command roundtrip succeeds
- bag recording pipeline starts

### 20.3 Workflow smoke tests

- minimal command/control smoke
- sensor topic smoke
- record/replay smoke

### 20.4 Heavy tests

- full mapping run
- benchmark report generation

Heavy tests should not all be CI-required at first, but they should exist.

## 21. CI strategy

CI should focus on what can run without the full simulator first.

Early CI candidates:

- lint
- unit tests
- launch syntax checks
- message generation
- report script tests

Simulator-in-the-loop tests may require:

- self-hosted runner
- manual workflow dispatch
- artifact upload

The project should not block on full simulator CI before the basic stack is
usable.

## 22. Risks and mitigations

### 22.1 Unreal and Windows environment drift

Risk:
the simulator toolchain may be old, brittle, or hard to recreate.

Mitigation:

- keep simulator-side changes minimal
- rely on packaged binaries where possible
- document exact versions

### 22.2 Communication layer opacity

Risk:
existing client interfaces may be poorly documented or tightly coupled.

Mitigation:

- isolate them behind a thin adapter
- capture protocol assumptions in docs
- create mocks for the ROS2 side

### 22.3 Time synchronization confusion

Risk:
simulator time and ROS time mismatches break SLAM and replay.

Mitigation:

- explicitly document timestamp source
- create timestamp validation tools early
- test replay workflows early

### 22.4 Sensor format mismatch

Risk:
LiDAR packets or metadata differ from ROS2 driver expectations.

Mitigation:

- add a narrow normalization layer
- preserve raw packet logging when possible
- validate packet rates and point counts

### 22.5 Scope explosion

Risk:
project expands into simulator development rather than integration.

Mitigation:

- keep milestones narrow
- defer simulator-side refactors
- treat bridge usability as the main success condition

## 23. License and redistribution policy

This workspace should keep a clean public distribution story.

Rules:

- default code in this workspace should be non-GPL
- simulator-side dependencies must be documented clearly
- externally sourced assets with separate licenses must be listed clearly
- participant-facing instructions must distinguish code license from scenario
  data license where needed

This is especially important if the workspace later becomes a benchmark or
challenge distribution point.

## 24. Success criteria

This project can be considered successful when the following become true:

- a user can follow the README and connect ROS2 to VTC
- the user can drive the robot with `/cmd_vel`
- the user can receive `/points_raw`
- the user can record rosbag2
- the user can replay the bag and generate a map
- the user can run a benchmark or report script
- the user does not need to understand Unreal internals to do the above

Longer-term success:

- external users reproduce the workflow
- scenarios become reusable benchmark tasks
- the workspace supports a virtual challenge concept

## 25. Immediate next tasks

### Task 1: Create README skeleton

The README should answer:

- what the project is
- what the project is not
- prerequisites
- quickstart
- current status

### Task 2: Inventory existing ROS1 API

Create a short document that lists:

- existing ROS1 topics
- services
- message types
- launch structure
- robot command interfaces
- sensor interfaces

This inventory is required before coding the ROS2 bridge.

### Task 3: Scaffold packages

Create the package skeletons:

- `vtc_msgs`
- `vtc_bridge`
- `vtc_bringup`
- `vtc_tools`

Even empty packages will help freeze the structure.

### Task 4: Define the minimum bridge loop

The first end-to-end loop should be:

- subscribe `/cmd_vel`
- send command to simulator
- receive pose/state
- publish `/odom`

This is the first true milestone.

### Task 5: Decide sensor path

Confirm whether the existing Velodyne packet path can be reused directly.
If yes, that becomes the standard sensor path.
If not, define the smallest adapter needed.

## 26. Near-term roadmap

### Next 1 to 2 sessions

- write README skeleton
- inventory ROS1 API
- create package scaffolding

### Next 3 to 5 sessions

- implement command/state bridge
- create bringup launch
- verify robot motion from ROS2

### Next 5 to 10 sessions

- establish LiDAR path
- record first bag
- replay and inspect sensor data

### Next 10+ sessions

- run SLAM
- generate first report
- define first virtual challenge scenario

## 27. Final note

The project should be judged by whether it creates a usable and reproducible
ROS2-facing VTC workflow, not by whether it fully modernizes the simulator.

If the bridge approach succeeds, it unlocks:

- lower-cost virtual validation
- reproducible LiDAR SLAM testing
- challenge-style benchmarking
- a practical development path that does not depend on constant field access

That is enough value to justify the project even before any full simulator port
is attempted.
