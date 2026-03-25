#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "sensor_msgs/msg/nav_sat_status.hpp"
#include "tf2_ros/static_transform_broadcaster.h"
#include "tf2_ros/transform_broadcaster.h"
#include "vtc_bridge/simulator_transport.hpp"
#include "vtc_msgs/msg/bridge_diagnostics.hpp"
#include "vtc_msgs/msg/vehicle_state.hpp"

using namespace std::chrono_literals;

namespace
{

geometry_msgs::msg::TransformStamped make_static_transform(
  const std::string & parent_frame,
  const std::string & child_frame,
  double x,
  double y,
  double z)
{
  geometry_msgs::msg::TransformStamped transform;
  transform.header.frame_id = parent_frame;
  transform.child_frame_id = child_frame;
  transform.transform.translation.x = x;
  transform.transform.translation.y = y;
  transform.transform.translation.z = z;
  transform.transform.rotation.w = 1.0;
  return transform;
}

}  // namespace

class VtcBridgeNode : public rclcpp::Node
{
public:
  VtcBridgeNode()
  : Node("vtc_bridge")
  {
    transport_backend_ = declare_parameter<std::string>("transport_backend", "mock");
    simulator_host_ = declare_parameter<std::string>("simulator_host", "127.0.0.1");
    vehicle_name_ = declare_parameter<std::string>("vehicle_name", "");
    cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "cmd_vel");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "odom");
    imu_topic_ = declare_parameter<std::string>("imu_topic", "imu/data");
    gnss_topic_ = declare_parameter<std::string>("gnss_topic", "gnss/fix");
    state_topic_ = declare_parameter<std::string>("state_topic", "vtc/state");
    diagnostics_topic_ = declare_parameter<std::string>("diagnostics_topic", "vtc/diagnostics");
    odom_frame_id_ = declare_parameter<std::string>("odom_frame_id", "odom");
    base_frame_id_ = declare_parameter<std::string>("base_frame_id", "base_link");
    imu_frame_id_ = declare_parameter<std::string>("imu_frame_id", "imu_link");
    lidar_frame_id_ = declare_parameter<std::string>("lidar_frame_id", "lidar3d_link");
    poll_rate_hz_ = declare_parameter<double>("poll_rate_hz", 50.0);
    diagnostics_rate_hz_ = declare_parameter<double>("diagnostics_rate_hz", 1.0);
    status_log_period_ms_ = declare_parameter<int>("status_log_period_ms", 10000);

    odom_pub_ = create_publisher<nav_msgs::msg::Odometry>(odom_topic_, 10);
    imu_pub_ = create_publisher<sensor_msgs::msg::Imu>(imu_topic_, 10);
    gnss_pub_ = create_publisher<sensor_msgs::msg::NavSatFix>(gnss_topic_, 10);
    state_pub_ = create_publisher<vtc_msgs::msg::VehicleState>(state_topic_, 10);
    diagnostics_pub_ = create_publisher<vtc_msgs::msg::BridgeDiagnostics>(diagnostics_topic_, 10);

    tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    static_tf_broadcaster_ = std::make_unique<tf2_ros::StaticTransformBroadcaster>(*this);
    publish_static_transforms();

    cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
      cmd_vel_topic_,
      rclcpp::SystemDefaultsQoS(),
      [this](const geometry_msgs::msg::Twist & msg) {
        ++commands_received_;
        last_command_linear_mps_ = msg.linear.x;
        last_command_angular_rps_ = msg.angular.z;

        if (!transport_) {
          last_error_ = "Transport backend is not available.";
          return;
        }

        if (!transport_->send_cmd_vel(msg.linear.x, msg.angular.z, last_error_)) {
          RCLCPP_WARN_THROTTLE(
            get_logger(),
            *get_clock(),
            2000,
            "Failed to forward cmd_vel to transport: %s",
            last_error_.c_str());
          return;
        }

        RCLCPP_INFO_THROTTLE(
          get_logger(),
          *get_clock(),
          2000,
          "Forwarded cmd_vel: linear.x=%.3f angular.z=%.3f",
          msg.linear.x,
          msg.angular.z);
      });

    initialize_transport();

    const auto poll_period = std::chrono::duration<double>(1.0 / std::max(1.0, poll_rate_hz_));
    poll_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::milliseconds>(poll_period),
      [this]() { poll_transport(); });

    const auto diagnostics_period = std::chrono::duration<double>(1.0 / std::max(0.1, diagnostics_rate_hz_));
    diagnostics_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::milliseconds>(diagnostics_period),
      [this]() { publish_diagnostics(now()); });

    reconnect_timer_ = create_wall_timer(2s, [this]() {
      if (!transport_ || transport_->is_connected()) {
        return;
      }
      initialize_transport();
    });
  }

private:
  void initialize_transport()
  {
    std::string error;
    if (!transport_) {
      transport_ = vtc_bridge::make_transport(transport_backend_, error);
      if (!transport_) {
        last_error_ = error;
        RCLCPP_ERROR(get_logger(), "%s", last_error_.c_str());
        return;
      }
    }

    if (transport_->connect(simulator_host_, vehicle_name_, error)) {
      last_error_.clear();
      RCLCPP_INFO(
        get_logger(),
        "Transport '%s' connected to simulator_host='%s' vehicle_name='%s'",
        transport_->backend_name().c_str(),
        simulator_host_.c_str(),
        vehicle_name_.c_str());
      return;
    }

    last_error_ = error;
    RCLCPP_WARN(get_logger(), "Transport connect failed: %s", last_error_.c_str());
  }

  void publish_static_transforms()
  {
    auto imu_transform = make_static_transform(base_frame_id_, imu_frame_id_, 0.0, 0.0, 0.0);
    auto lidar_transform = make_static_transform(base_frame_id_, lidar_frame_id_, -0.22, 0.0, 0.518);
    const auto stamp = now();
    imu_transform.header.stamp = stamp;
    lidar_transform.header.stamp = stamp;
    static_tf_broadcaster_->sendTransform({imu_transform, lidar_transform});
  }

  void poll_transport()
  {
    if (!transport_ || !transport_->is_connected()) {
      return;
    }

    vtc_bridge::TransportState state;
    if (!transport_->poll(state, last_error_)) {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        2000,
        "Transport poll failed: %s",
        last_error_.c_str());
      return;
    }

    ++state_updates_;
    last_sim_time_sec_ = state.simulator_time_sec;
    const auto stamp = now();

    publish_odom(stamp, state);
    publish_imu(stamp, state);
    publish_gnss(stamp, state);
    publish_vehicle_state(stamp, state);
    publish_dynamic_tf(stamp, state);

    RCLCPP_INFO_THROTTLE(
      get_logger(),
      *get_clock(),
      status_log_period_ms_,
      "Bridge active: backend=%s host=%s state_updates=%zu commands=%zu sim_time=%.3f",
      transport_->backend_name().c_str(),
      simulator_host_.c_str(),
      state_updates_,
      commands_received_,
      last_sim_time_sec_);
  }

  void publish_odom(const rclcpp::Time & stamp, const vtc_bridge::TransportState & state)
  {
    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp;
    odom.header.frame_id = odom_frame_id_;
    odom.child_frame_id = base_frame_id_;
    odom.pose.pose.position.x = state.position_m[0];
    odom.pose.pose.position.y = state.position_m[1];
    odom.pose.pose.position.z = state.position_m[2];
    odom.pose.pose.orientation.x = state.orientation_xyzw[0];
    odom.pose.pose.orientation.y = state.orientation_xyzw[1];
    odom.pose.pose.orientation.z = state.orientation_xyzw[2];
    odom.pose.pose.orientation.w = state.orientation_xyzw[3];
    odom.twist.twist.linear.x = state.linear_velocity_mps;
    odom.twist.twist.angular.z = state.angular_velocity_rps[2];
    odom_pub_->publish(odom);
  }

  void publish_imu(const rclcpp::Time & stamp, const vtc_bridge::TransportState & state)
  {
    sensor_msgs::msg::Imu imu;
    imu.header.stamp = stamp;
    imu.header.frame_id = imu_frame_id_;
    imu.orientation.x = state.orientation_xyzw[0];
    imu.orientation.y = state.orientation_xyzw[1];
    imu.orientation.z = state.orientation_xyzw[2];
    imu.orientation.w = state.orientation_xyzw[3];
    imu.angular_velocity.x = state.angular_velocity_rps[0];
    imu.angular_velocity.y = state.angular_velocity_rps[1];
    imu.angular_velocity.z = state.angular_velocity_rps[2];
    imu.linear_acceleration.x = state.linear_acceleration_mps2[0];
    imu.linear_acceleration.y = state.linear_acceleration_mps2[1];
    imu.linear_acceleration.z = state.linear_acceleration_mps2[2];
    imu_pub_->publish(imu);
  }

  void publish_gnss(const rclcpp::Time & stamp, const vtc_bridge::TransportState & state)
  {
    sensor_msgs::msg::NavSatFix fix;
    fix.header.stamp = stamp;
    fix.header.frame_id = base_frame_id_;
    fix.status.service = sensor_msgs::msg::NavSatStatus::SERVICE_GPS;
    fix.status.status = state.gnss_valid ?
      sensor_msgs::msg::NavSatStatus::STATUS_FIX :
      sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
    fix.latitude = state.latitude_deg;
    fix.longitude = state.longitude_deg;
    fix.altitude = state.altitude_m;
    gnss_pub_->publish(fix);
  }

  void publish_vehicle_state(const rclcpp::Time & stamp, const vtc_bridge::TransportState & state)
  {
    vtc_msgs::msg::VehicleState msg;
    msg.stamp = stamp;
    msg.simulator_host = simulator_host_;
    msg.vehicle_name = vehicle_name_;
    msg.connected = transport_ && transport_->is_connected();
    msg.sim_time = state.simulator_time_sec;
    msg.linear_velocity = state.linear_velocity_mps;
    msg.angular_velocity = state.angular_velocity_rps[2];
    msg.left_wheel_rpm = state.wheel_rpm[0];
    msg.right_wheel_rpm = state.wheel_rpm[1];
    msg.pose.position.x = state.position_m[0];
    msg.pose.position.y = state.position_m[1];
    msg.pose.position.z = state.position_m[2];
    msg.pose.orientation.x = state.orientation_xyzw[0];
    msg.pose.orientation.y = state.orientation_xyzw[1];
    msg.pose.orientation.z = state.orientation_xyzw[2];
    msg.pose.orientation.w = state.orientation_xyzw[3];
    msg.linear_acceleration.x = state.linear_acceleration_mps2[0];
    msg.linear_acceleration.y = state.linear_acceleration_mps2[1];
    msg.linear_acceleration.z = state.linear_acceleration_mps2[2];
    msg.angular_velocity_vector.x = state.angular_velocity_rps[0];
    msg.angular_velocity_vector.y = state.angular_velocity_rps[1];
    msg.angular_velocity_vector.z = state.angular_velocity_rps[2];
    msg.latitude = state.latitude_deg;
    msg.longitude = state.longitude_deg;
    msg.altitude = state.altitude_m;
    msg.gnss_valid = state.gnss_valid;
    state_pub_->publish(msg);
  }

  void publish_diagnostics(const rclcpp::Time & stamp)
  {
    vtc_msgs::msg::BridgeDiagnostics msg;
    msg.stamp = stamp;
    msg.transport_backend = transport_backend_;
    msg.simulator_host = simulator_host_;
    msg.vehicle_name = vehicle_name_;
    msg.connected = transport_ && transport_->is_connected();
    msg.commands_received = commands_received_;
    msg.state_updates = state_updates_;
    msg.last_sim_time = last_sim_time_sec_;
    msg.last_error = last_error_;
    diagnostics_pub_->publish(msg);
  }

  void publish_dynamic_tf(const rclcpp::Time & stamp, const vtc_bridge::TransportState & state)
  {
    geometry_msgs::msg::TransformStamped transform;
    transform.header.stamp = stamp;
    transform.header.frame_id = odom_frame_id_;
    transform.child_frame_id = base_frame_id_;
    transform.transform.translation.x = state.position_m[0];
    transform.transform.translation.y = state.position_m[1];
    transform.transform.translation.z = state.position_m[2];
    transform.transform.rotation.x = state.orientation_xyzw[0];
    transform.transform.rotation.y = state.orientation_xyzw[1];
    transform.transform.rotation.z = state.orientation_xyzw[2];
    transform.transform.rotation.w = state.orientation_xyzw[3];
    tf_broadcaster_->sendTransform(transform);
  }

  std::string transport_backend_;
  std::string simulator_host_;
  std::string vehicle_name_;
  std::string cmd_vel_topic_;
  std::string odom_topic_;
  std::string imu_topic_;
  std::string gnss_topic_;
  std::string state_topic_;
  std::string diagnostics_topic_;
  std::string odom_frame_id_;
  std::string base_frame_id_;
  std::string imu_frame_id_;
  std::string lidar_frame_id_;
  double poll_rate_hz_;
  double diagnostics_rate_hz_;
  int status_log_period_ms_;
  size_t commands_received_{0};
  size_t state_updates_{0};
  double last_command_linear_mps_{0.0};
  double last_command_angular_rps_{0.0};
  double last_sim_time_sec_{0.0};
  std::string last_error_;
  std::unique_ptr<vtc_bridge::SimulatorTransport> transport_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr gnss_pub_;
  rclcpp::Publisher<vtc_msgs::msg::VehicleState>::SharedPtr state_pub_;
  rclcpp::Publisher<vtc_msgs::msg::BridgeDiagnostics>::SharedPtr diagnostics_pub_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  std::unique_ptr<tf2_ros::StaticTransformBroadcaster> static_tf_broadcaster_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
  rclcpp::TimerBase::SharedPtr diagnostics_timer_;
  rclcpp::TimerBase::SharedPtr reconnect_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VtcBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
