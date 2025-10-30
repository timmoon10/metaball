#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace metaball {

/*! \brief Numerical integrator on unit interval */
class Integrator {
 public:
  using ScalarType = double;

  virtual ~Integrator() = default;

  virtual std::string describe() const = 0;

  virtual ScalarType operator()(
      const std::function<ScalarType(ScalarType)>& integrand) const = 0;

  static std::unique_ptr<Integrator> make_integrator(
      const std::string_view& config);
};

class TrapezoidIntegrator : public Integrator {
 public:
  TrapezoidIntegrator(size_t num_evals);

  std::string describe() const override;

  ScalarType operator()(
      const std::function<ScalarType(ScalarType)>& integrand) const override;

 private:
  size_t num_evals_;
};

class MonteCarloIntegrator : public Integrator {
 public:
  MonteCarloIntegrator(size_t num_evals);

  std::string describe() const override;

  ScalarType operator()(
      const std::function<ScalarType(ScalarType)>& integrand) const override;

 private:
  size_t num_evals_;
};

}  // namespace metaball
