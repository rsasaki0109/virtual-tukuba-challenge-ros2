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
- ROS2 package の雛形は未作成
- simulator 実機接続確認は未実施

## Near-Term Tasks

1. `vtc_msgs`, `vtc_bridge`, `vtc_bringup`, `vtc_tools` の雛形を作る
2. ROS1 I/F と ROS2 I/F の対応を固定する
3. 最小bridgeで `cmd_vel -> setVW`, `getStatusOne -> odom/tf` を実装する
4. LiDAR path を packaged binary で再確認する

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

まだ quickstart を提供できる段階ではありません。

現時点では、以下を前提に進めます。

1. Windows 側で VTC packaged binary を起動する
2. Linux / ROS2 側で bridge を起動する
3. LiDAR / odom / tf を確認する
4. rosbag2 を記録する
5. replay と SLAM を行う

## Notes

- 現状は設計とI/F確認フェーズです
- topic名やframe名は ROS1 実装と差異がある可能性があるため、ROS2 側で明示的に固定します
- 時刻系は `simClock` と受信時刻のどちらを採用するか未決定です
