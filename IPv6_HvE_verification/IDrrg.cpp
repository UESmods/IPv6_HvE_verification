#include "IPv6_HvE_verification.h"

#include <iostream>
#include <random>
#include <ctime>

using namespace std;

/**
 * @brief 生成指定范围 [min_val, max_val] 内的均匀分布随机整数。
 * * @param min_val 随机数的最小值（包含）。
 * @param max_val 随机数的最大值（包含）。
 * @return int 在 [min_val, max_val] 范围内的随机整数。
 */
int generate_random_int(int min_val, int max_val) {
    if (min_val > max_val) {
        swap(min_val, max_val);
    }

    // 1. 使用当前时间作为种子，创建一个随机数引擎。
    // 注意：在现代 C++ 中，推荐使用 std::random_device 来获取更好的种子，
    // 但为了确保在所有环境中都能编译和运行，这里使用 std::time(nullptr) 作为回退。
    // 在实际应用中，应尽量将引擎和分布对象声明为静态或全局，以避免重复播种。
    static mt19937 generator(static_cast<unsigned int>(time(nullptr)));

    // 2. 定义一个均匀整数分布，确保生成的数字在 [min_val, max_val] 范围内。
    uniform_int_distribution<int> distribution(min_val, max_val);

    // 3. 生成并返回随机数。
    return distribution(generator);
}