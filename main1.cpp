#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <random>
#include <future>
#include <cstdlib>
#include <cassert>
#include "clicker.hh"

template< class It, class F >
size_t omp_parallel(F f, It data, size_t s)
{
  size_t res = 0;
  #pragma omp parallel for reduction(+:res)
  for (size_t i = 0; i < s; ++i)
  {
    res += f(data[i], i);
  }
  return res;
}

template< class It, class F >
size_t future_loop(F f, It data, size_t start, size_t size)
{
  size_t res = 0;
  for (size_t i = start; i < (start + size); ++i)
  {
    res += f(data[i], i);
  }
  return res;
}

template< class It, class F >
size_t future_parallel(size_t thrs, F f, It data, size_t s)
{
  std::vector< std::future< size_t > > futures;
  futures.reserve(thrs - 1);
  size_t per_th = s / thrs;
  for (size_t i = 0; i < thrs - 1; ++i)
  {
    futures.emplace_back(
      std::async(future_loop< It, F >, f, data, i * per_th, per_th)
    );
  }
  size_t last_th = (thrs - 1) * per_th;
  auto res = future_loop(f, data, last_th, s - last_th);
  for (auto && ft: futures)
  {
    res += ft.get();
  }
  return res;
}

size_t threen_calc(size_t v, size_t)
{
  size_t next = 0;
  while (v != 1)
  {
    if (v % 2)
    {
      v = 3 * v + 1;
    }
    else
    {
      v /= 2;
    }
    ++next;
  }
  return next;
}
size_t wo(size_t & v, size_t i)
{
  return v = i;
}
size_t ro(size_t v, size_t)
{
  return v;
}

size_t threeN(size_t thrs, const size_t * data, size_t s)
{
  return future_parallel(thrs, threen_calc, data, s);
}
size_t omp_threeN(const size_t * data, size_t s)
{
  return omp_parallel(threen_calc, data, s);
}

size_t write_only(size_t thrs, size_t * data, size_t s)
{
  return future_parallel(thrs, wo, data, s);
}

size_t read_only(size_t thrs, const size_t * data, size_t s)
{
  return future_parallel(thrs, ro, data, s);
}

int main(int argc, char ** argv)
{
  assert((argc == 3 || argc == 4)
    && "size of seq expected and ti, wi, ri or o12");
  size_t vals = std::strtoull(argv[1], nullptr, 10);
  std::string mode = argv[2];
  using f_t = std::function< size_t(size_t *, size_t) >;
  std::unordered_map< std::string, f_t > hmap;
  using namespace std::placeholders;
  hmap["r1"] = std::bind(read_only, 1, _1, _2);
  hmap["r2"] = std::bind(read_only, 2, _1, _2);
  hmap["r6"] = std::bind(read_only, 6, _1, _2);
  hmap["r12"] = std::bind(read_only, 12, _1, _2);
  hmap["t1"] = std::bind(threeN, 1, _1, _2);
  hmap["t2"] = std::bind(threeN, 2, _1, _2);
  hmap["t6"] = std::bind(threeN, 6, _1, _2);
  hmap["t12"] = std::bind(threeN, 12, _1, _2);
  hmap["w1"] = std::bind(write_only, 1, _1, _2);
  hmap["w2"] = std::bind(write_only, 2, _1, _2);
  hmap["w6"] = std::bind(write_only, 6, _1, _2);
  hmap["w12"] = std::bind(write_only, 12, _1, _2);
  hmap["o12"] = omp_threeN;
  try
  {
    hmap.at(mode);
  }
  catch (...)
  {
    std::cerr << "Wrong mode\n";
    return 1;
  }
  std::vector< size_t > data(vals, 0);
  size_t max = argc == 4 ? std::strtoull(argv[3], nullptr, 10) : 4;
  std::default_random_engine eng;
  std::uniform_int_distribution< size_t > dv(1, max);
  std::generate_n(data.begin(), data.size(),
    [&]()
    {
      return dv(eng);
    });
  mtt::Clicker clk;
  volatile size_t res = hmap[mode](data.data(), data.size());
  auto result = clk.millisec();
  std::cout << "In time: " << result << " milliseconds\n";
  std::cout << "Memory: ";
  auto mems = (sizeof(size_t) * vals) / (1024 * 1024);
  std::cout << mems << " MB\n";
  auto speed = (1000.0f * mems / 1024) / result;
  std::cout << std::setprecision(5) << speed << " GB/sec\n";
}
