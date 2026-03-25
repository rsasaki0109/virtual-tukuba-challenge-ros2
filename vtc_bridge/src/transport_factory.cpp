#include "vtc_bridge/simulator_transport.hpp"

#include <memory>
#include <string>

namespace vtc_bridge
{

std::unique_ptr<SimulatorTransport> make_transport(const std::string & backend, std::string & error)
{
  if (backend == "mock") {
    error.clear();
    return make_mock_transport();
  }

#ifdef VTC_BRIDGE_HAS_CAGECLIENT
  if (backend == "cageclient") {
    error.clear();
    return make_cageclient_transport();
  }
#endif

  error = "Unsupported transport backend: " + backend;
  return nullptr;
}

}  // namespace vtc_bridge
