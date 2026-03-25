# virtual_tukuba_challenge_v2_ws

Virtual Tsukuba Challenge (VTC) を ROS2 から使うための bridge / bringup / tooling をまとめるワークスペースです。

## What This Is

- VTC を ROS2 から操作するための統合ワークスペース
- rosbag2 記録、LiDAR SLAM、評価スクリプトまでを再現可能にするための土台
- 既存の VTC / CageClient / sensor path を極力そのまま使うための接続層

## What This Is Not

- VTC 本体を全面的に ROS2 ネイティブ化するrepoではありません
- Unreal Engine ベースのシミュレータ本体forkを主目的にしたrepoではありません
- フル自律スタックや競技運営基盤を最初から含むrepoではありません

## Repository Strategy

- 主要repoはこの workspace 側に置く
- `furo-org/VTC` は当面外部依存として扱う
- simulator-side 修正は、本当に不足APIが確認された時点で最小forkを検討する
- 修正候補は `VTC` 本体だけでなく `CageClient` / `CagePlugin` も含めて判断する

詳細メモ: [`notes/2026-03-26_vtc_ros1_api_inventory.md`](./notes/2026-03-26_vtc_ros1_api_inventory.md)

## Planned Scope

最小構成の目標は次のとおりです。

- `/cmd_vel` で robot command を送る
- `/odom` と `/tf` を受ける
- LiDAR を `/points_raw` に載せる
- rosbag2 を記録する
- replay から SLAM と map/report 生成まで通す

## External Dependencies

想定している外部依存は次のとおりです。

- `furo-org/VTC`
- `furo-org/CageClient`
- 既存の Velodyne 互換 sensor path
- Linux 上の ROS2 環境
- Windows 上の VTC 実行環境

## Current Status

- `plan.md` を作成済み
- 公開ROS1 APIの棚卸し初版を作成済み
- `vtc_msgs` / `vtc_bridge` / `vtc_bringup` / `vtc_tools` の雛形を作成済み
- `vtc_bridge` には mock backend と conditional な `cageclient` backend を実装済み
- simulator 実機接続確認は未実施

## Near-Term Tasks

1. `transport_backend:=cageclient` を実環境で有効化して packaged VTC と接続確認する
2. ROS1 I/F と ROS2 I/F の対応を固定する
3. LiDAR path を packaged binary で再確認する
4. `vtc_msgs` に control service 定義を追加する

## Proposed Layout

```text
virtual_tukuba_challenge_v2_ws/
├── plan.md
├── README.md
├── notes/
├── vtc_msgs/
├── vtc_bridge/
├── vtc_bringup/
└── vtc_tools/
```

## Quickstart

最小雛形の build と起動だけなら可能です。

```bash
colcon build --symlink-install
source install/setup.bash
ros2 launch vtc_bringup minimal_bridge.launch.py
```

現時点の `vtc_bridge_node` は mock transport を使って `odom`、`imu/data`、
`gnss/fix`、`vtc/state`、`vtc/diagnostics`、`tf` を publish する scaffold です。
まだ VTC 本体には接続しません。

`transport_backend:=cageclient` は実装済みですが、build 時に以下が揃っている場合だけ
有効になります。

- `third_party/CageClient` に `CageClient` checkout がある
- `libzmq3-dev` がインストールされている

例:

```bash
git clone --recursive https://github.com/furo-org/CageClient.git third_party/CageClient
sudo apt install libzmq3-dev
colcon build --symlink-install
source install/setup.bash
ros2 launch vtc_bringup minimal_bridge.launch.py transport_backend:=cageclient simulator_host:=192.168.1.110
```

実際の VTC 連携フローは引き続き以下を目標にします。

1. Windows 側で VTC packaged binary を起動する
2. Linux / ROS2 側で bridge を起動する
3. LiDAR / odom / tf を確認する
4. rosbag2 を記録する
5. replay と SLAM を行う

## Notes

- 現状は設計とI/F確認フェーズです
- topic名やframe名は ROS1 実装と差異がある可能性があるため、ROS2 側で明示的に固定します
- 時刻系は `simClock` と受信時刻のどちらを採用するか未決定です
