# 2026-03-26 VTC ROS1 API棚卸しメモ

## 結論

- `/media/sasaki/aiueo/ai_coding_ws/virtual_tukuba_challenge_v2_ws/plan.md` の方針どおり、初期段階では `furo-org/VTC` を主軸repoとしてforkしない方がよい。
- 公開されている `VTC` + `CageClient` + `cage_ros_stack` の組み合わせだけで、少なくとも「コマンド送信」「状態取得」「LiDAR受信開始」は成立する根拠がある。
- ただし、本体無改造が確定したわけではない。`reset/pause/resume`、シナリオ情報、時刻意味論、追加センサなどをROS2側で必要とした時点でAPI不足が出る可能性がある。
- その場合の修正先は `VTC` 本体とは限らない。先に `CageClient` または `CagePlugin` 側を疑うべき。

## 確認済み事実

- 事実: `VTC` のREADMEには、「既存の環境でロボットを走らせるだけなら、パッケージ済みバイナリと `CageClient` または `cage_ros_stack` を使う」と明記されている。
  - 参照repo: `https://github.com/furo-org/VTC`
  - 確認commit: `c503a8827cb1134a4931fd93cb1af093df5ba35f`
- 事実: `VTC` は `CagePlugin`、`ZMQUE`、`PxArticulationLink` をsubmoduleとして利用している。
  - 参照repo: `https://github.com/furo-org/VTC`
  - 確認commit: `c503a8827cb1134a4931fd93cb1af093df5ba35f`
- 事実: `cage_ros_bridge` は `cmd_vel` を購読し、`odom`、`odom_gt`、`gps/fix`、`imu` をpublishしている。あわせて `odom -> base_link`、`odom -> base_link_gt`、`base_link -> imu_link`、`base_link -> lidar3d_link` のTFをpublishしている。
  - 参照repo: `https://github.com/furo-org/cage_ros_stack`
  - 確認commit: `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`
- 事実: `cage_ros_stack` の `bringup.launch` は、`cage_ros_bridge`、`rviz`、`rostopic`、`velodyne_pointcloud` の `VLP16_points.launch` を起動する。起動時に `rostopic pub -1 /cmd_vel ...` を送ってLiDAR送信を開始させる構成になっている。
  - 参照repo: `https://github.com/furo-org/cage_ros_stack`
  - 確認commit: `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`
- 事実: `cage_ros_bridge` 自体はレーザスキャンデータを扱わず、VTCシミュレータが「コマンドを送ってきたホスト」に対してVLP-16相当プロトコルでスキャンデータを送る前提になっている。
  - 参照repo: `https://github.com/furo-org/cage_ros_stack`
  - 確認commit: `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`
- 事実: `CageClient` の公開APIには `connect()`、`getStatusOne()`、`setVW()`、`setRpm()`、`setFLW()` があり、`vehicleStatus` には `simClock`、左右RPM、加速度、角速度、姿勢、ワールド座標、緯度経度が含まれる。
  - 参照repo: `https://github.com/furo-org/CageClient`
  - 確認commit: `7c3507e5d4c1279fab2f618ca1e168365fd2bb29`
- 事実: `CageClient` は接続時に `VehicleInfo` と `WorldInfo` を取得し、車輪パラメータ、各種Transform、地理基準点情報を保持する。
  - 参照repo: `https://github.com/furo-org/CageClient`
  - 確認commit: `7c3507e5d4c1279fab2f618ca1e168365fd2bb29`
- 事実: ROS1 bridgeのREADMEと実装コードにはtopic名の不一致がある。READMEは ` /fix/gps ` を説明しているが、実装コードは `gps/fix` をpublishしている。
  - 参照repo: `https://github.com/furo-org/cage_ros_stack`
  - 確認commit: `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`
- 事実: 今回確認した範囲では、ROS serviceやactionに相当する公開I/Fは `cage_ros_stack` 側に存在しない。
  - 参照repo: `https://github.com/furo-org/cage_ros_stack`
  - 確認commit: `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`

## 推論

- 推論: ROS2最小bridgeの初版は、`CageClient` 互換adapterを作り、`/cmd_vel` から `setVW()`、`getStatusOne()` から `/odom`、`/imu/data`、`/gnss/fix` への変換を行う構成で十分に開始できる可能性が高い。
- 推論: `/vtc/reset`、`/vtc/pause`、`/vtc/resume`、`/vtc/get_scenario_info` を `/media/sasaki/aiueo/ai_coding_ws/virtual_tukuba_challenge_v2_ws/plan.md` どおり実装したい場合、追加APIが必要になる可能性が高い。
- 推論: 追加APIが必要になった場合、修正対象は `VTC` ルートrepoより、通信境界に近い `CageClient` または `CagePlugin` の方が筋がよい。

## 未確認/要確認項目

- `simClock` をROS2の標準timestampとして使えるか。現行ROS1 bridgeは `ros::Time::now()` を使っており、`simClock` をheader stampに使っていない。
- `reset/pause/resume` を外部から安全に呼べる公開I/Fが既にあるか。
- シナリオ情報、診断情報、接続状態を取得する既存I/Fがあるか。
- 現行パッケージ済みVTCバイナリでも `WorldInfo` とTransform群が常に取得できるか。
- LiDAR packetの送信条件、送信先IP/port、packet loss時の挙動を現行バイナリで再確認する必要がある。

## 次アクション

1. `/media/sasaki/aiueo/ai_coding_ws/virtual_tukuba_challenge_v2_ws/README.md` の骨子を作り、「VTCは外部依存」「forkは不足APIが確認されるまで保留」を明記する。
2. ROS1 I/F からROS2 I/F への対応表を作る。特に `imu -> /imu/data`、`gps/fix -> /gnss/fix`、`odom_gt` の扱いを決める。
3. `vtc_msgs`、`vtc_bridge`、`vtc_bringup`、`vtc_tools` の雛形を作る前に、最小bridgeの入出力を `cmd_vel / odom / tf / points_raw` に固定する。
4. 実機確認では、まずVTC packaged binary + `CageClient`/`cage_ros_stack` 相当経路で「接続」「停止コマンド送信」「LiDAR受信開始」「状態取得」を検証する。

## 参照

- `https://github.com/furo-org/VTC` at `c503a8827cb1134a4931fd93cb1af093df5ba35f`
- `https://github.com/furo-org/CageClient` at `7c3507e5d4c1279fab2f618ca1e168365fd2bb29`
- `https://github.com/furo-org/cage_ros_stack` at `a3c7dda05d2cde2fe48f10bc81b14e51dc3d5c51`
