#pragma once

#include <array>
#include <memory>
#include <string>

namespace vtc_bridge
{

struct TransportState
{
  double simulator_time_sec{0.0};
  std::array<double, 3> position_m{0.0, 0.0, 0.0};
  std::array<double, 4> orientation_xyzw{0.0, 0.0, 0.0, 1.0};
  std::array<double, 2> wheel_rpm{0.0, 0.0};
  std::array<double, 3> linear_acceleration_mps2{0.0, 0.0, 0.0};
  std::array<double, 3> angular_velocity_rps{0.0, 0.0, 0.0};
  double linear_velocity_mps{0.0};
  double latitude_deg{0.0};
  double longitude_deg{0.0};
  double altitude_m{0.0};
  bool gnss_valid{false};
};

class SimulatorTransport
{
public:
  virtual ~SimulatorTransport() = default;

  virtual std::string backend_name() const = 0;
  virtual bool connect(
    const std::string & simulator_host,
    const std::string & vehicle_name,
    std::string & error) = 0;
  virtual bool is_connected() const = 0;
  virtual bool send_cmd_vel(double linear_velocity_mps, double angular_velocity_rps, std::string & error) = 0;
  virtual bool poll(TransportState & state, std::string & error) = 0;
};

std::unique_ptr<SimulatorTransport> make_mock_transport();
std::unique_ptr<SimulatorTransport> make_cageclient_transport();
std::unique_ptr<SimulatorTransport> make_transport(const std::string & backend, std::string & error);

}  // namespace vtc_bridge
