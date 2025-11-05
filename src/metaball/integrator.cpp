#include "metaball/integrator.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "metaball/random.hpp"
#include "util/error.hpp"
#include "util/string.hpp"

namespace metaball {

std::unique_ptr<Integrator> Integrator::make_integrator(
    const std::string_view& config) {
  const auto config_parsed = util::split(config, "=", 2);
  UTIL_CHECK(config_parsed.size() == 1 || config_parsed.size() == 2,
             "error parsing config (", config, ")");
  const auto& type = util::strip(config_parsed[0]);
  const auto& params =
      config_parsed.size() > 1 ? util::strip(config_parsed[1]) : "";
  if (type == "grid") {
    const size_t num_evals =
        params.empty() ? 64 : util::from_string<size_t>(params);
    return std::make_unique<GridIntegrator>(num_evals);
  }
  if (type == "trapezoid") {
    const size_t num_evals =
        params.empty() ? 64 : util::from_string<size_t>(params);
    return std::make_unique<TrapezoidIntegrator>(num_evals);
  }
  if (type == "monte carlo") {
    const size_t num_evals =
        params.empty() ? 64 : util::from_string<size_t>(params);
    return std::make_unique<MonteCarloIntegrator>(num_evals);
  }
  if (type == "stratified sampling") {
    const size_t num_grids =
        params.empty() ? 64 : util::from_string<size_t>(params);
    return std::make_unique<StratifiedSamplingIntegrator>(num_grids, 1);
  }
  UTIL_ERROR("Unrecognized integrator (", type, ")");
}

GridIntegrator::GridIntegrator(size_t num_evals) : num_evals_{num_evals} {}

std::string GridIntegrator::describe() const {
  return util::concat_strings("GridIntegrator (num_evals=", num_evals_, ")");
}

GridIntegrator::ScalarType GridIntegrator::operator()(
    const std::function<ScalarType(ScalarType)>& integrand) const {
  UTIL_CHECK(num_evals_ >= 1,
             "Grid integration requires at least 1 evaluation point, but got ",
             num_evals_);
  const ScalarType half_grid_size = static_cast<ScalarType>(0.5) / num_evals_;
  ScalarType result = 0;
  for (size_t i = 0; i < num_evals_; ++i) {
    result += integrand(half_grid_size * (2 * i + 1));
  }
  result *= 2 * half_grid_size;
  return result;
}

TrapezoidIntegrator::TrapezoidIntegrator(size_t num_evals)
    : num_evals_{num_evals} {
  UTIL_CHECK(num_evals_ >= 2,
             "Trapezoid rule requires at least 2 evaluation points, but got ",
             num_evals_);
}

std::string TrapezoidIntegrator::describe() const {
  return util::concat_strings("TrapezoidIntegrator (num_evals=", num_evals_,
                              ")");
}

TrapezoidIntegrator::ScalarType TrapezoidIntegrator::operator()(
    const std::function<ScalarType(ScalarType)>& integrand) const {
  constexpr ScalarType zero = static_cast<ScalarType>(0);
  constexpr ScalarType one = static_cast<ScalarType>(1);
  const ScalarType grid_size = one / (num_evals_ - 1);
  ScalarType result = integrand(zero) / 2;
  for (size_t i = 1; i < num_evals_ - 1; ++i) {
    result += integrand(grid_size * i);
  }
  result += integrand(one) / 2;
  result *= grid_size;
  return result;
}

MonteCarloIntegrator::MonteCarloIntegrator(size_t num_evals)
    : num_evals_{num_evals} {}

std::string MonteCarloIntegrator::describe() const {
  return util::concat_strings("MonteCarloIntegrator (num_evals=", num_evals_,
                              ")");
}

MonteCarloIntegrator::ScalarType MonteCarloIntegrator::operator()(
    const std::function<ScalarType(ScalarType)>& integrand) const {
  ScalarType result = 0;
  for (size_t i = 0; i < num_evals_; ++i) {
    result += integrand(random::rand<ScalarType>());
  }
  result /= num_evals_;
  return result;
}

StratifiedSamplingIntegrator::StratifiedSamplingIntegrator(
    size_t num_grids, size_t evals_per_grid)
    : num_grids_{num_grids}, evals_per_grid_{evals_per_grid} {}

std::string StratifiedSamplingIntegrator::describe() const {
  return util::concat_strings(
      "StratifiedSamplingIntegrator (num_grids=", num_grids_,
      ", evals_per_grid=", evals_per_grid_, ")");
}

StratifiedSamplingIntegrator::ScalarType
StratifiedSamplingIntegrator::operator()(
    const std::function<ScalarType(ScalarType)>& integrand) const {
  const ScalarType grid_size = static_cast<ScalarType>(1) / num_grids_;
  ScalarType result = 0;
  for (size_t i = 0; i < num_grids_; ++i) {
    const ScalarType offset = grid_size * i;
    for (size_t j = 0; j < evals_per_grid_; ++j) {
      result += integrand(offset + grid_size * random::rand<ScalarType>());
    }
  }
  result /= num_grids_ * evals_per_grid_;
  return result;
}

}  // namespace metaball
