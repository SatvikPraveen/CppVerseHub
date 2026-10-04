/**
 * @file MathUtils.cpp
 * @brief Implementation of the numerical toolkit declared in MathUtils.hpp.
 */
#include "utils/MathUtils.hpp"

#include <iomanip>
#include <numeric>

namespace CppVerseHub::Utils::Math {

// ===================================================================================================
// Matrix
// ===================================================================================================

Matrix::Matrix(std::size_t rows, std::size_t cols) : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

Matrix::Matrix(std::initializer_list<std::initializer_list<double>> rows)
    : rows_(rows.size()), cols_(rows.size() == 0 ? 0 : rows.begin()->size()) {
    data_.reserve(rows_ * cols_);
    for (const auto& row : rows) {
        if (row.size() != cols_) {
            throw std::invalid_argument("Matrix: ragged initializer list");
        }
        data_.insert(data_.end(), row.begin(), row.end());
    }
}

Matrix Matrix::identity(std::size_t n) {
    Matrix m(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        m(i, i) = 1.0;
    }
    return m;
}

double Matrix::at(std::size_t r, std::size_t c) const {
    if (r >= rows_ || c >= cols_) {
        throw std::out_of_range("Matrix::at: index out of range");
    }
    return (*this)(r, c);
}

Matrix Matrix::transposed() const {
    Matrix t(cols_, rows_);
    for (std::size_t r = 0; r < rows_; ++r) {
        for (std::size_t c = 0; c < cols_; ++c) {
            t(c, r) = (*this)(r, c);
        }
    }
    return t;
}

double Matrix::determinant() const {
    if (!isSquare()) {
        throw std::logic_error("Matrix::determinant: matrix is not square");
    }
    Matrix lu = *this;
    const std::size_t n = rows_;
    double det = 1.0;
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t pivot = k;
        for (std::size_t r = k + 1; r < n; ++r) {
            if (std::abs(lu(r, k)) > std::abs(lu(pivot, k))) {
                pivot = r;
            }
        }
        if (lu(pivot, k) == 0.0) {
            return 0.0;
        }
        if (pivot != k) {
            for (std::size_t c = 0; c < n; ++c) {
                std::swap(lu(k, c), lu(pivot, c));
            }
            det = -det;
        }
        det *= lu(k, k);
        for (std::size_t r = k + 1; r < n; ++r) {
            const double factor = lu(r, k) / lu(k, k);
            for (std::size_t c = k; c < n; ++c) {
                lu(r, c) -= factor * lu(k, c);
            }
        }
    }
    return det;
}

std::optional<Matrix> Matrix::inverse() const {
    if (!isSquare()) {
        throw std::logic_error("Matrix::inverse: matrix is not square");
    }
    const std::size_t n = rows_;
    Matrix a = *this;
    Matrix inv = identity(n);
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t pivot = k;
        for (std::size_t r = k + 1; r < n; ++r) {
            if (std::abs(a(r, k)) > std::abs(a(pivot, k))) {
                pivot = r;
            }
        }
        if (std::abs(a(pivot, k)) < 1e-12) {
            return std::nullopt;
        }
        for (std::size_t c = 0; c < n; ++c) {
            std::swap(a(k, c), a(pivot, c));
            std::swap(inv(k, c), inv(pivot, c));
        }
        const double diag = a(k, k);
        for (std::size_t c = 0; c < n; ++c) {
            a(k, c) /= diag;
            inv(k, c) /= diag;
        }
        for (std::size_t r = 0; r < n; ++r) {
            if (r == k) {
                continue;
            }
            const double factor = a(r, k);
            if (factor == 0.0) {
                continue;
            }
            for (std::size_t c = 0; c < n; ++c) {
                a(r, c) -= factor * a(k, c);
                inv(r, c) -= factor * inv(k, c);
            }
        }
    }
    return inv;
}

std::optional<std::vector<double>> Matrix::solve(std::span<const double> b) const {
    if (!isSquare() || b.size() != rows_) {
        throw std::invalid_argument("Matrix::solve: dimension mismatch");
    }
    const std::size_t n = rows_;
    Matrix a = *this;
    std::vector<double> x(b.begin(), b.end());
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t pivot = k;
        for (std::size_t r = k + 1; r < n; ++r) {
            if (std::abs(a(r, k)) > std::abs(a(pivot, k))) {
                pivot = r;
            }
        }
        if (std::abs(a(pivot, k)) < 1e-12) {
            return std::nullopt;
        }
        if (pivot != k) {
            for (std::size_t c = 0; c < n; ++c) {
                std::swap(a(k, c), a(pivot, c));
            }
            std::swap(x[k], x[pivot]);
        }
        for (std::size_t r = k + 1; r < n; ++r) {
            const double factor = a(r, k) / a(k, k);
            for (std::size_t c = k; c < n; ++c) {
                a(r, c) -= factor * a(k, c);
            }
            x[r] -= factor * x[k];
        }
    }
    for (std::size_t i = n; i-- > 0;) {
        double sum = x[i];
        for (std::size_t c = i + 1; c < n; ++c) {
            sum -= a(i, c) * x[c];
        }
        x[i] = sum / a(i, i);
    }
    return x;
}

double Matrix::trace() const {
    if (!isSquare()) {
        throw std::logic_error("Matrix::trace: matrix is not square");
    }
    double t = 0.0;
    for (std::size_t i = 0; i < rows_; ++i) {
        t += (*this)(i, i);
    }
    return t;
}

Matrix operator*(const Matrix& a, const Matrix& b) {
    if (a.cols_ != b.rows_) {
        throw std::invalid_argument("Matrix multiply: dimension mismatch");
    }
    Matrix result(a.rows_, b.cols_);
    // i-k-j loop order keeps the inner loop contiguous for both operands.
    for (std::size_t i = 0; i < a.rows_; ++i) {
        for (std::size_t k = 0; k < a.cols_; ++k) {
            const double aik = a(i, k);
            for (std::size_t j = 0; j < b.cols_; ++j) {
                result(i, j) += aik * b(k, j);
            }
        }
    }
    return result;
}

std::vector<double> operator*(const Matrix& a, std::span<const double> v) {
    if (a.cols_ != v.size()) {
        throw std::invalid_argument("Matrix-vector multiply: dimension mismatch");
    }
    std::vector<double> result(a.rows_, 0.0);
    for (std::size_t r = 0; r < a.rows_; ++r) {
        for (std::size_t c = 0; c < a.cols_; ++c) {
            result[r] += a(r, c) * v[c];
        }
    }
    return result;
}

Matrix operator+(const Matrix& a, const Matrix& b) {
    if (a.rows_ != b.rows_ || a.cols_ != b.cols_) {
        throw std::invalid_argument("Matrix add: dimension mismatch");
    }
    Matrix r = a;
    for (std::size_t i = 0; i < r.data_.size(); ++i) {
        r.data_[i] += b.data_[i];
    }
    return r;
}

Matrix operator-(const Matrix& a, const Matrix& b) {
    if (a.rows_ != b.rows_ || a.cols_ != b.cols_) {
        throw std::invalid_argument("Matrix subtract: dimension mismatch");
    }
    Matrix r = a;
    for (std::size_t i = 0; i < r.data_.size(); ++i) {
        r.data_[i] -= b.data_[i];
    }
    return r;
}

Matrix operator*(const Matrix& a, double s) {
    Matrix r = a;
    for (auto& v : r.data_) {
        v *= s;
    }
    return r;
}

bool Matrix::approxEquals(const Matrix& other, double epsilon) const noexcept {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        return false;
    }
    for (std::size_t i = 0; i < data_.size(); ++i) {
        if (!approxEqual(data_[i], other.data_[i], epsilon)) {
            return false;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const Matrix& m) {
    for (std::size_t r = 0; r < m.rows_; ++r) {
        os << '[';
        for (std::size_t c = 0; c < m.cols_; ++c) {
            os << (c ? ", " : "") << m(r, c);
        }
        os << "]\n";
    }
    return os;
}

Vec3 rotateZ(const Vec3& v, double radians) noexcept {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return Vec3{c * v.x() - s * v.y(), s * v.x() + c * v.y(), v.z()};
}

Vec3 rotateAroundAxis(const Vec3& v, const Vec3& axis, double radians) noexcept {
    if (axis.lengthSquared() == 0.0) {
        return v;
    }
    const Vec3 k = axis.normalized();
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return v * c + k.cross(v) * s + k * (k.dot(v) * (1.0 - c));
}

// ===================================================================================================
// Number theory / numerical methods
// ===================================================================================================

std::vector<std::uint64_t> NumberTheory::primesUpTo(std::uint64_t limit) {
    std::vector<std::uint64_t> primes;
    if (limit < 2) {
        return primes;
    }
    std::vector<bool> composite(static_cast<std::size_t>(limit) + 1, false);
    for (std::uint64_t i = 2; i <= limit; ++i) {
        if (composite[static_cast<std::size_t>(i)]) {
            continue;
        }
        primes.push_back(i);
        for (std::uint64_t j = i * i; j <= limit; j += i) {
            composite[static_cast<std::size_t>(j)] = true;
        }
    }
    return primes;
}

double Numerical::integrateSimpson(const std::function<double(double)>& f, double a, double b,
                                   std::size_t intervals) {
    std::size_t n = std::max<std::size_t>(intervals, 2);
    if (n % 2 != 0) {
        ++n;
    }
    const double h = (b - a) / static_cast<double>(n);
    double sum = f(a) + f(b);
    for (std::size_t i = 1; i < n; ++i) {
        sum += f(a + h * static_cast<double>(i)) * (i % 2 == 0 ? 2.0 : 4.0);
    }
    return sum * h / 3.0;
}

std::optional<double> Numerical::newtonRaphson(const std::function<double(double)>& f,
                                               const std::function<double(double)>& df, double x0,
                                               double tolerance, int maxIterations) {
    double x = x0;
    for (int i = 0; i < maxIterations; ++i) {
        const double d = df(x);
        if (d == 0.0 || !std::isfinite(d)) {
            return std::nullopt;
        }
        const double step = f(x) / d;
        x -= step;
        if (!std::isfinite(x)) {
            return std::nullopt;
        }
        if (std::abs(step) < tolerance) {
            return x;
        }
    }
    return std::nullopt;
}

std::optional<double> Numerical::bisection(const std::function<double(double)>& f, double lo, double hi,
                                           double tolerance) {
    double flo = f(lo);
    const double fhi = f(hi);
    if (flo == 0.0) {
        return lo;
    }
    if (fhi == 0.0) {
        return hi;
    }
    if ((flo < 0.0) == (fhi < 0.0)) {
        return std::nullopt;
    }
    for (int i = 0; i < 200 && (hi - lo) > tolerance; ++i) {
        const double mid = lo + (hi - lo) / 2.0;
        const double fmid = f(mid);
        if (fmid == 0.0) {
            return mid;
        }
        if ((fmid < 0.0) == (flo < 0.0)) {
            lo = mid;
            flo = fmid;
        } else {
            hi = mid;
        }
    }
    return lo + (hi - lo) / 2.0;
}

// ===================================================================================================
// Statistics
// ===================================================================================================

namespace {
void requireNonEmpty(std::span<const double> data, const char* what) {
    if (data.empty()) {
        throw std::invalid_argument(std::string{what} + ": empty data set");
    }
}
} // namespace

double Statistics::mean(std::span<const double> data) {
    requireNonEmpty(data, "mean");
    return std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(data.size());
}

double Statistics::variance(std::span<const double> data, bool sample) {
    requireNonEmpty(data, "variance");
    if (sample && data.size() < 2) {
        throw std::invalid_argument("variance: sample variance needs at least two values");
    }
    const double m = mean(data);
    double sumSq = 0.0;
    double sumDiff = 0.0;
    for (const double v : data) {
        sumSq += (v - m) * (v - m);
        sumDiff += v - m;
    }
    const auto n = static_cast<double>(data.size());
    // Corrected two-pass formula (Chan, Golub & LeVeque) compensates for rounding in the mean.
    const double corrected = sumSq - sumDiff * sumDiff / n;
    return corrected / (sample ? n - 1.0 : n);
}

double Statistics::standardDeviation(std::span<const double> data, bool sample) {
    return std::sqrt(variance(data, sample));
}

double Statistics::median(std::span<const double> data) {
    return percentile(data, 50.0);
}

double Statistics::percentile(std::span<const double> data, double p) {
    requireNonEmpty(data, "percentile");
    if (p < 0.0 || p > 100.0) {
        throw std::invalid_argument("percentile: p must be within [0, 100]");
    }
    std::vector<double> sorted(data.begin(), data.end());
    std::sort(sorted.begin(), sorted.end());
    const double rank = p / 100.0 * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(rank));
    const auto upper = static_cast<std::size_t>(std::ceil(rank));
    return Interpolation::lerp(sorted[lower], sorted[upper], rank - static_cast<double>(lower));
}

double Statistics::correlation(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("correlation: need two equal-length series with >= 2 values");
    }
    const double mx = mean(x);
    const double my = mean(y);
    double sxy = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sxy += (x[i] - mx) * (y[i] - my);
        sxx += (x[i] - mx) * (x[i] - mx);
        syy += (y[i] - my) * (y[i] - my);
    }
    if (sxx == 0.0 || syy == 0.0) {
        return 0.0;
    }
    return sxy / std::sqrt(sxx * syy);
}

Statistics::LinearFit Statistics::linearRegression(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("linearRegression: need two equal-length series with >= 2 values");
    }
    const double mx = mean(x);
    const double my = mean(y);
    double sxy = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sxy += (x[i] - mx) * (y[i] - my);
        sxx += (x[i] - mx) * (x[i] - mx);
        syy += (y[i] - my) * (y[i] - my);
    }
    if (sxx == 0.0) {
        throw std::invalid_argument("linearRegression: x values are all identical");
    }
    LinearFit fit;
    fit.slope = sxy / sxx;
    fit.intercept = my - fit.slope * mx;
    fit.rSquared = syy == 0.0 ? 1.0 : (sxy * sxy) / (sxx * syy);
    return fit;
}

// ===================================================================================================
// Random / noise
// ===================================================================================================

Vec3 RandomGenerator::unitVector() {
    // Marsaglia-style: uniform z and azimuth give a uniform distribution on the sphere.
    const double z = uniformReal(-1.0, 1.0);
    const double phi = uniformReal(0.0, Constants::kTau);
    const double r = std::sqrt(std::max(0.0, 1.0 - z * z));
    return Vec3{r * std::cos(phi), r * std::sin(phi), z};
}

PerlinNoise::PerlinNoise(std::uint64_t seed) {
    std::array<std::uint8_t, 256> p{};
    for (std::size_t i = 0; i < p.size(); ++i) {
        p[i] = static_cast<std::uint8_t>(i);
    }
    // Explicit Fisher-Yates with the engine's raw output keeps the table identical on every platform.
    std::mt19937_64 engine{seed};
    for (std::size_t i = p.size() - 1; i > 0; --i) {
        const auto j = static_cast<std::size_t>(engine() % (i + 1));
        std::swap(p[i], p[j]);
    }
    for (std::size_t i = 0; i < perm_.size(); ++i) {
        perm_[i] = p[i & 255U];
    }
}

namespace {
constexpr double fade(double t) noexcept {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

constexpr double grad(std::uint8_t hash, double x, double y, double z) noexcept {
    const unsigned h = hash & 15U;
    const double u = h < 8U ? x : y;
    const double v = h < 4U ? y : (h == 12U || h == 14U ? x : z);
    return ((h & 1U) == 0U ? u : -u) + ((h & 2U) == 0U ? v : -v);
}
} // namespace

double PerlinNoise::noise(double x, double y, double z) const noexcept {
    const double fx = std::floor(x);
    const double fy = std::floor(y);
    const double fz = std::floor(z);
    const auto xi = static_cast<std::size_t>(static_cast<long long>(fx) & 255);
    const auto yi = static_cast<std::size_t>(static_cast<long long>(fy) & 255);
    const auto zi = static_cast<std::size_t>(static_cast<long long>(fz) & 255);
    x -= fx;
    y -= fy;
    z -= fz;
    const double u = fade(x);
    const double v = fade(y);
    const double w = fade(z);

    const std::size_t a = perm_[xi] + yi;
    const std::size_t aa = perm_[a] + zi;
    const std::size_t ab = perm_[a + 1] + zi;
    const std::size_t b = perm_[xi + 1] + yi;
    const std::size_t ba = perm_[b] + zi;
    const std::size_t bb = perm_[b + 1] + zi;

    using Interpolation::lerp;
    return lerp(lerp(lerp(grad(perm_[aa], x, y, z), grad(perm_[ba], x - 1, y, z), u),
                     lerp(grad(perm_[ab], x, y - 1, z), grad(perm_[bb], x - 1, y - 1, z), u), v),
                lerp(lerp(grad(perm_[aa + 1], x, y, z - 1), grad(perm_[ba + 1], x - 1, y, z - 1), u),
                     lerp(grad(perm_[ab + 1], x, y - 1, z - 1), grad(perm_[bb + 1], x - 1, y - 1, z - 1), u),
                     v),
                w);
}

double PerlinNoise::fractal(double x, double y, double z, int octaves, double persistence) const noexcept {
    double total = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double maxValue = 0.0;
    for (int i = 0; i < std::max(octaves, 1); ++i) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    return total / maxValue;
}

// ===================================================================================================
// Geometry
// ===================================================================================================

double Geometry::polygonArea(std::span<const Vec2> polygon) noexcept {
    if (polygon.size() < 3) {
        return 0.0;
    }
    double twice = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        twice += polygon[i].cross(polygon[(i + 1) % polygon.size()]);
    }
    return twice / 2.0;
}

bool Geometry::pointInPolygon(const Vec2& p, std::span<const Vec2> polygon) noexcept {
    bool inside = false;
    const std::size_t n = polygon.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Vec2& a = polygon[i];
        const Vec2& b = polygon[j];
        if ((a.y() > p.y()) != (b.y() > p.y()) &&
            p.x() < (b.x() - a.x()) * (p.y() - a.y()) / (b.y() - a.y()) + a.x()) {
            inside = !inside;
        }
    }
    return inside;
}

std::vector<Vec2> Geometry::convexHull(std::vector<Vec2> points) {
    std::sort(points.begin(), points.end(), [](const Vec2& a, const Vec2& b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() < 3) {
        return points;
    }
    std::vector<Vec2> hull(points.size() * 2);
    std::size_t k = 0;
    for (const auto& p : points) {
        while (k >= 2 && (hull[k - 1] - hull[k - 2]).cross(p - hull[k - 2]) <= 0.0) {
            --k;
        }
        hull[k++] = p;
    }
    const std::size_t lowerSize = k + 1;
    for (std::size_t i = points.size() - 1; i-- > 0;) {
        const auto& p = points[i];
        while (k >= lowerSize && (hull[k - 1] - hull[k - 2]).cross(p - hull[k - 2]) <= 0.0) {
            --k;
        }
        hull[k++] = p;
    }
    hull.resize(k - 1);
    return hull;
}

namespace {
int orientation(const Vec2& a, const Vec2& b, const Vec2& c) noexcept {
    const double v = (b - a).cross(c - a);
    if (std::abs(v) < 1e-12) {
        return 0;
    }
    return v > 0.0 ? 1 : -1;
}
bool onSegment(const Vec2& a, const Vec2& b, const Vec2& p) noexcept {
    return std::min(a.x(), b.x()) <= p.x() && p.x() <= std::max(a.x(), b.x()) &&
           std::min(a.y(), b.y()) <= p.y() && p.y() <= std::max(a.y(), b.y());
}
} // namespace

bool Geometry::segmentsIntersect(const Vec2& p1, const Vec2& p2, const Vec2& q1, const Vec2& q2) noexcept {
    const int o1 = orientation(p1, p2, q1);
    const int o2 = orientation(p1, p2, q2);
    const int o3 = orientation(q1, q2, p1);
    const int o4 = orientation(q1, q2, p2);
    if (o1 != o2 && o3 != o4) {
        return true;
    }
    return (o1 == 0 && onSegment(p1, p2, q1)) || (o2 == 0 && onSegment(p1, p2, q2)) ||
           (o3 == 0 && onSegment(q1, q2, p1)) || (o4 == 0 && onSegment(q1, q2, p2));
}

double Geometry::pointSegmentDistance(const Vec2& p, const Vec2& a, const Vec2& b) noexcept {
    const Vec2 ab = b - a;
    const double len2 = ab.lengthSquared();
    if (len2 == 0.0) {
        return p.distanceTo(a);
    }
    const double t = std::clamp((p - a).dot(ab) / len2, 0.0, 1.0);
    return p.distanceTo(a + ab * t);
}

// ===================================================================================================
// Orbital mechanics
// ===================================================================================================

double Space::orbitalPeriod(double semiMajorAxis, double mu) {
    return Constants::kTau * std::sqrt(semiMajorAxis * semiMajorAxis * semiMajorAxis / mu);
}

double Space::circularVelocity(double radius, double mu) {
    return std::sqrt(mu / radius);
}

double Space::escapeVelocity(double radius, double mu) {
    return std::sqrt(2.0 * mu / radius);
}

Space::HohmannTransfer Space::hohmannTransfer(double r1, double r2, double mu) {
    if (r1 <= 0.0 || r2 <= 0.0 || mu <= 0.0) {
        throw std::invalid_argument("hohmannTransfer: radii and mu must be positive");
    }
    const double a = (r1 + r2) / 2.0;
    HohmannTransfer t;
    t.deltaV1 = std::sqrt(mu / r1) * (std::sqrt(2.0 * r2 / (r1 + r2)) - 1.0);
    t.deltaV2 = std::sqrt(mu / r2) * (1.0 - std::sqrt(2.0 * r1 / (r1 + r2)));
    t.totalDeltaV = std::abs(t.deltaV1) + std::abs(t.deltaV2);
    t.transferTime = Constants::kPi * std::sqrt(a * a * a / mu);
    return t;
}

std::size_t Space::NBodySimulator::addBody(const Body& body) {
    bodies_.push_back(body);
    accelerationsValid_ = false;
    return bodies_.size() - 1;
}

void Space::NBodySimulator::computeAccelerations() {
    accelerations_.assign(bodies_.size(), Vec3{});
    const double eps2 = softening_ * softening_;
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies_.size(); ++j) {
            const Vec3 d = bodies_[j].position - bodies_[i].position;
            const double r2 = d.lengthSquared() + eps2;
            if (r2 == 0.0) {
                continue;
            }
            const double invR3 = 1.0 / (r2 * std::sqrt(r2));
            accelerations_[i] += d * (g_ * bodies_[j].mass * invR3);
            accelerations_[j] -= d * (g_ * bodies_[i].mass * invR3);
        }
    }
    accelerationsValid_ = true;
}

void Space::NBodySimulator::step(double dt) {
    if (!accelerationsValid_) {
        computeAccelerations();
    }
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i].velocity += accelerations_[i] * (dt / 2.0);
        bodies_[i].position += bodies_[i].velocity * dt;
    }
    computeAccelerations();
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i].velocity += accelerations_[i] * (dt / 2.0);
    }
}

double Space::NBodySimulator::totalEnergy() const {
    double kinetic = 0.0;
    double potential = 0.0;
    const double eps2 = softening_ * softening_;
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        kinetic += 0.5 * bodies_[i].mass * bodies_[i].velocity.lengthSquared();
        for (std::size_t j = i + 1; j < bodies_.size(); ++j) {
            const double r = std::sqrt((bodies_[j].position - bodies_[i].position).lengthSquared() + eps2);
            if (r > 0.0) {
                potential -= g_ * bodies_[i].mass * bodies_[j].mass / r;
            }
        }
    }
    return kinetic + potential;
}

Vec3 Space::NBodySimulator::totalMomentum() const noexcept {
    Vec3 p{};
    for (const auto& b : bodies_) {
        p += b.velocity * b.mass;
    }
    return p;
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateMath(std::ostream& out) {
    out << "=== Math utilities ===\n";
    constexpr Vec3 a{1.0, 0.0, 0.0};
    constexpr Vec3 b{0.0, 1.0, 0.0};
    constexpr Vec3 c = a.cross(b);
    static_assert(c == Vec3{0.0, 0.0, 1.0}, "constexpr cross product");
    out << "cross((1,0,0),(0,1,0)) = " << c << '\n';
    out << "rotateZ((1,0,0), 90 deg) = " << std::fixed << std::setprecision(3) << rotateZ(a, toRadians(90.0))
        << '\n';

    const Matrix m{{4.0, 7.0}, {2.0, 6.0}};
    out << "det([[4,7],[2,6]]) = " << m.determinant() << '\n';
    if (const auto inv = m.inverse()) {
        out << "inverse =\n" << *inv;
    }
    const std::vector<double> rhs{1.0, 2.0};
    if (const auto x = m.solve(rhs)) {
        out << "solve A x = (1,2) -> x = (" << (*x)[0] << ", " << (*x)[1] << ")\n";
    }

    static_assert(NumberTheory::isPrime(1'000'000'007ULL), "constexpr primality");
    static_assert(NumberTheory::factorial(10).value() == 3'628'800ULL, "constexpr factorial");
    out << "primes <= 30: ";
    for (const auto p : NumberTheory::primesUpTo(30)) {
        out << p << ' ';
    }
    out << "\nC(52,5) = " << NumberTheory::binomial(52, 5) << ", F(50) = " << NumberTheory::fibonacci(50)
        << '\n';

    const double integral = Numerical::integrateSimpson([](double x) { return std::sin(x); }, 0.0,
                                                        Constants::kPi);
    out << "integral of sin over [0, pi] = " << std::setprecision(6) << integral << '\n';
    if (const auto root = Numerical::newtonRaphson([](double x) { return x * x - 2.0; },
                                                   [](double x) { return 2.0 * x; }, 1.0)) {
        out << "Newton sqrt(2) = " << std::setprecision(10) << *root << '\n';
    }

    const std::vector<double> samples{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    out << std::setprecision(3) << "mean = " << Statistics::mean(samples)
        << ", population stddev = " << Statistics::standardDeviation(samples, false)
        << ", median = " << Statistics::median(samples) << '\n';

    RandomGenerator rng{2024};
    out << "random d6 rolls:";
    for (int i = 0; i < 5; ++i) {
        out << ' ' << rng.uniformInt(1, 6);
    }
    out << '\n';

    const PerlinNoise noise{7};
    out << "Perlin fractal(0.5, 0.25) = " << noise.fractal(0.5, 0.25, 0.0, 4) << '\n';

    const std::vector<Vec2> square{{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}};
    out << "square area = " << Geometry::polygonArea(square) << ", contains (1,1): " << std::boolalpha
        << Geometry::pointInPolygon(Vec2{1.0, 1.0}, square) << '\n';

    const double leo = Constants::kEarthRadius + 400e3;
    const double geo = 42'164e3;
    const auto transfer = Space::hohmannTransfer(leo, geo);
    out << "LEO->GEO Hohmann: total dv = " << std::setprecision(1) << transfer.totalDeltaV
        << " m/s, time = " << transfer.transferTime / 3600.0 << " h\n";

    Space::NBodySimulator sim{1.0};
    sim.addBody({Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, 0.0}, 1.0});
    sim.addBody({Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0}, 1e-6});
    const double e0 = sim.totalEnergy();
    for (int i = 0; i < 1000; ++i) {
        sim.step(0.001 * Constants::kTau);
    }
    out << "N-body one orbit: relative energy drift = " << std::scientific << std::setprecision(2)
        << std::abs((sim.totalEnergy() - e0) / e0) << '\n';
    out << std::defaultfloat << std::setprecision(6) << std::noboolalpha;
}

} // namespace CppVerseHub::Utils::Math
