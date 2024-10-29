#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <random>
#include <cstdlib>
#include <cassert>
#include "clicker.hh"

size_t threeN(const size_t * data, size_t s)
{
  size_t results = 0;
  #pragma omp parallel for reduction(+:results)
  for (size_t i = 0; i < s; ++i)
  {
    size_t v = data[i];
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
    results += next;
  }
  return results;
}

size_t write_only(size_t * data, size_t s)
{
  size_t sum = 0;
  #pragma omp parallel for reduction(+:sum)
  for (size_t i = 0; i < s; ++i)
  {
    sum += data[i] = i;
  }
  return sum;
}

size_t read_only(const size_t * data, size_t s)
{
  int sum = 0;
  #pragma omp parallel for reduction(+: sum)
  for (size_t i = 0; i < s; ++i)
  {
    sum += data[i];
  }
  return sum;
}

int main(int argc, char ** argv)
{
  assert((argc == 3 || argc == 4) && "size of seq expected and t, w or r");
  size_t vals = std::strtoull(argv[1], nullptr, 10);
  std::string mode = argv[2];
  std::unordered_map< std::string, std::function< decltype(write_only) > > hmap;
  hmap["r"] = read_only;
  hmap["t"] = threeN;
  hmap["w"] = write_only;
  try
  {
    hmap.at(mode);
  }
  catch (...)
  {
    std::cerr << "Wrong mode (t, w or r)\n";
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
