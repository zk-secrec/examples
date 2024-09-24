# Approximating transcendental functions

## Introduction

This example presents ZK-SecreC code for evaluating certain mathematical functions in the statement that we want to prove in zero-knowledge. The mathematical functions include all the algebraic functions (including powers and roots), as well as certain transcendental functions: trigonometric and exponential functions and their inverses. The arguments and values of the functions are represented as fixed-point numbers, available in the standard library of ZK-SecreC. The algebraic functions are evaluated directly: any algebraic function can be precisely computed under zero-knowledge up to the precision limit of representing the numbers. The methods for evaluating the transcendental functions are based on the [approximation methods that we have introduced](https://ia.cr/2024/859).

## Implementation in ZK-SecreC

`implicit_algebraic_function_fixed` in `NewFixpElemfuns.zksc` is the main ZK-SecreC function for evaluating algebraic functions. Its arguments are the argument `x`, on which to evaluate the function, the coefficients `coeffs` of the bivariate polynomial **P** defining the algebraic function (`coeffs[i][j]` is the coefficient of `X^i * Y^j`), and the lower and upper bounds, between which the function will look for the value `y` satisfying **P**(`x`,`y`)=0. The function `implicit_algebraic_function_fixed` also takes size asserters as an argument (see the documentation about the implementation of inequality checks in the standard library of ZK-SecreC).

`ElemFuncs.zksc` contains the functions for evaluating trigonometric and exponential functions and their inverses. `ElemFuncsTest.zksc` demonstrates, how these can be called. Evaluating a transcendental function is done in two steps, the result of the first of which can be reused.

1. Read in the coefficients defining the algebraic function that approximates the transcendental function that you want to evaluate. This is done by the function `get_params`. It takes as parameters the name of the transcendental function that we want to approximate, as well as the desired upper bound on approximation error; smaller bound implies that a higher-degree algebraic function must be used. The parameters read by `get_params` must be among the public parameters of the program, with certain names for these parameters. The file `alg_param_file.json` contains these parameters.
    * The possible function names that `get_params` recognizes, are `"log2_shifted"`, `"sin_quarter"`, `"log2_erfc_scaled"`, `"exp2"`, and `"arcsin"`.
1. Call the actual function, giving it the argument `x`, as well as the parameters gathered during the previous step. The functions are defined in `ElemFuncs.zksc`. Which public parameters to gather during the previous step, depends on the function that we want to call in this step; this dependency is stated below. Note that the function `fixed_log2_erfc_scaled` computes the function _s_ in [our writeup](https://ia.cr/2024/859).

| Actual function being called | The parameters that `get_params` must collect |
| --- | --- |
| `fixed_sin` | `"sin_quarter"` |
| `fixed_exp2` | `"exp2"` |
| `fixed_exp` | `"exp2"` |
| `fixed_log2` | `"log2_shifted"` |
| `fixed_ln` | `"log2_shifted"` |
| `fixed_log10` | `"log2_shifted"` |
| `fixed_log2_erfc_scaled` | `"log2_erfc_scaled"` |
| `fixed_arcsin` | `"arcsin"` |
| `fixed_arccos` | `"arcsin"` |

