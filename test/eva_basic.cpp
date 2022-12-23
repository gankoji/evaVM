#include <gtest/gtest.h>
#include "math_ops.h"
#include "eva_symbols.h"
#include "comparison_ops.h"
#include "branching.h"
#include "variables.h"
#include "localvars.h"

TEST(EvaBasic, SanityCheck)
{
    EXPECT_EQ("hi", "hi");
}