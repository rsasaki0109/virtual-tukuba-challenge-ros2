#include "vtc_bridge/simulator_transport.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <string>

namespace vtc_bridge
{
namespace
{

class MockSimulatorTransport : public SimulatorTransport
{
public:
  MockSimulatorTransport() = default;

  std::string backend_name() const override
  {
    return "mock";
  }

  bool connect(
    const std::string & simulator_host,
    const std::string & vehicle_name,
    std::string & error) override
  {
    simulator_host_ = simulator_host;
    vehicle_name_ = vehicle_name.empty() ? "mock_vehicle" : vehicle_name;
    last_update_ = std::chrono::steady_clock::now();
    connected_ = true;
    error.clear();
    return true;
  }

  bool is_connected() const override
  {
    return connected_;
  }

  bool send_cmd_vel(double linear_velocity_mps, double angular_velocity_rps, std::string & error) override
  {
    if (!connected_) {
      error = "Mock transport is not connected.";
      return false;
    }

    linear_velocity_mps_ = linear_velocity_mps;
    angular_velocity_rps_ = angular_velocity_rps;
    error.clear();
    return true;
  }

  bool poll(TransportState & state, std::string & error) override
  {
    if (!connected_) {
      error = "Mock transport is not connected.";
      return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto dt = std::chrono::duration<double>(now - last_update_).count();
    last_update_ = now;

    if (dt > 0.0) {
      yaw_rad_ += angular_velocity_rps_ * dt;
      x_m_ += std::cos(yaw_rad_) * linear_velocity_mps_ * dt;
      y_m_ += std::sin(yaw_rad_) * linear_velocity_mps_ * dt;
      simulator_time_sec_ += dt;
    }

    const double right_wheel_velocity = linear_velocity_mps_ + (angular_velocity_rps_ * track_width_m_ / 2.0);
    const double left_wheel_velocity = linear_velocity_mps_ - (angular_velocity_rps_ * track_width_m_ / 2.0);
    const double wheel_circumference_m = 2.0 * M_PI * wheel_radius_m_;

    state.simulator_time_sec = simulator_time_sec_;
    state.position_m = {x_m_, y_m_, 0.0};
    state.orientation_xyzw = {0.0, 0.0, std::sin(yaw_rad_ / 2.0), std::cos(yaw_rad_ / 2.0)};
    state.wheel_rpm = {
      (left_wheel_velocity / wheel_circumference_m) * 60.0,
      (right_wheel_velocity / wheel_circumference_m) * 60.0
    };
    state.linear_acceleration_mps2 = {0.0, 0.0, 0.0};
    state.angular_velocity_rps = {0.0, 0.0, angular_velocity_rps_};
    state.linear_velocity_mps = linear_velocity_mps_;
    state.latitude_deg = latitude_origin_deg_ + (y_m_ / meters_per_degree_latitude_);
    state.longitude_deg = longitude_origin_deg_ +
      (x_m_ / (meters_per_degree_latitude_ * std::cos(latitude_origin_deg_ * M_PI / 180.0)));
    state.altitude_m = 0.0;
    state.gnss_valid = true;

    error.clear();
    return true;
  }

private:
  bool connected_{false};
  std::string simulator_host_;
  std::string vehicle_name_;
  std::chrono::steady_clock::time_point last_update_{};
  double simulator_time_sec_{0.0};
  double x_m_{0.0};
  double y_m_{0.0};
  double yaw_rad_{0.0};
  double linear_velocity_mps_{0.0};
  double angular_velocity_rps_{0.0};

  static constexpr double track_width_m_ = 0.5;
  static constexpr double wheel_radius_m_ = 0.15;
  static constexpr double latitude_origin_deg_ = 36.0825;
  static constexpr double longitude_origin_deg_ = 140.1113;
  static constexpr double meters_per_degree_latitude_ = 111111.0;
};

}  // namespace

std::unique_ptr<SimulatorTransport> make_mock_transport()
{
  return std::make_unique<MockSimulatorTransport>();
}

}  // namespace vtc_bridge
