
#include <iostream>

/* from the fit library (github.com/pfultz2/Fit) */
#define EXPECT_TRUE(...) if (!(__VA_ARGS__)) { std::cout << "***FAILED:  EXPECT_TRUE(" << #__VA_ARGS__ << ")\n     File: " << __FILE__ << ": " << __LINE__ << '\n'; return 1; }

#define EXECUTE_TEST(...) if ((__VA_ARGS__) != 0) { return 1; } else { std::cout << "ok " << #__VA_ARGS__ << '\n'; }
