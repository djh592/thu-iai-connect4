#include <cmath>

double bonus(const int n_i, const int N)
{
    return std::sqrt((std::log((double)N) / (double)n_i));
}

double UCB(const double value, const double C, const double bonus)
{
    return value + C * bonus;
}