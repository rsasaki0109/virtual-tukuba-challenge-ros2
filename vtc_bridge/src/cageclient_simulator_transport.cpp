#include "vtc_bridge/simulator_transport.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <string>

#include "cageclient.hh"

namespace vtc_bridge
{
namespace
{

class CageClientTransport : public SimulatorTransport
{
public:
  CageClientTransport() = default;

  std::string backend_name() const override
  {
    return "cageclient";
  }

  bool connect(
    const std::string & simulator_host,
    const std::string & vehicle_name,
    std::string & error) override
  {
    simulator_host_ = simulator_host;
    vehicle_name_ = vehicle_name;

    cage_ = std::make_unique<CageAPI>(simulator_host, vehicle_name);
    cage_->setDefaultTransform("Lidar", {-.22, 0, .518}, {0, 0, 0, 1});
    cage_->setDefaultTransform("IMU", {0, 0, 0}, {0, 0, 0, 1});

    if (!cage_->connect()) {
      error = cage_->getError();
      return false;
    }

    connected_ = true;
    error.clear();
    return true;
  }

  bool is_connected() const override
  {
    return connected_ && cage_ != nullptr;
  }

  bool send_cmd_vel(double linear_velocity_mps, double angular_velocity_rps, std::string & error) override
  {
    if (!is_connected()) {
      error = "CageClient transport is not connected.";
      return false;
    }

    if (!cage_->setVW(linear_velocity_mps, angular_velocity_rps)) {
      error = cage_->getError();
      return false;
    }

    error.clear();
    return true;
  }

  bool poll(TransportState & state, std::string & error) override
  {
    if (!is_connected()) {
      error = "CageClient transport is not connected.";
      return false;
    }

    CageAPI::vehicleStatus raw_state;
    if (!cage_->getStatusOne(raw_state, poll_timeout_us_)) {
      error = cage_->getError();
      return false;
    }

    state.simulator_time_sec = raw_state.simClock;
    state.position_m = {raw_state.wx, raw_state.wy, raw_state.wz};
    state.orientation_xyzw = {raw_state.ox, raw_state.oy, raw_state.oz, raw_state.ow};
    state.wheel_rpm = {raw_state.lrpm, raw_state.rrpm};
    state.linear_acceleration_mps2 = {raw_state.ax, raw_state.ay, raw_state.az};
    state.angular_velocity_rps = {raw_state.rx, raw_state.ry, raw_state.rz};
    state.linear_velocity_mps = calculate_linear_velocity(raw_state);
    state.latitude_deg = raw_state.latitude;
    state.longitude_deg = raw_state.longitude;
    state.altitude_m = std::numeric_limits<double>::quiet_NaN();
    state.gnss_valid = cage_->WorldInfo.valid && std::isfinite(raw_state.latitude) &&
      std::isfinite(raw_state.longitude);

    error.clear();
    return true;
  }

private:
  double calculate_linear_velocity(const CageAPI::vehicleStatus & raw_state) const
  {
    if (!cage_) {
      return 0.0;
    }

    const auto & info = cage_->VehicleInfo;
    if (info.ReductionRatio == 0.0) {
      return 0.0;
    }

    const double right_velocity =
      -raw_state.rrpm * info.WheelPerimeterR / info.ReductionRatio / 60.0;
    const double left_velocity =
      raw_state.lrpm * info.WheelPerimeterL / info.ReductionRatio / 60.0;
    return (right_velocity + left_velocity) / 2.0;
  }

  bool connected_{false};
  std::string simulator_host_;
  std::string vehicle_name_;
  std::unique_ptr<CageAPI> cage_;
  static constexpr int poll_timeout_us_ = 10000;
};

}  // namespace

std::unique_ptr<SimulatorTransport> make_cageclient_transport()
{
  return std::make_unique<CageClientTransport>();
}

}  // namespace vtc_bridge
