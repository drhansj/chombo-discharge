# Test cases

A collection of tests with the goal of narrowing down the triple junction MG solver issues.

All should run in 2D with MPI locally in a matter of seconds. Should run with 3D, untested.

## Geometry
Consists of 3 parts, 
1. Dielectric, cylinder along y-axis
2. Ground, half embedded in y-hi side of dielectric
3. Electrode, hald embedded in y-lo side of dielectric

## Boundary conditions
x-lo, x-hi, y-hi all with homogeneous Dirichlet conditions
y-lo with homogeneous Neumann condition

## Cases

- Base working cases. Using a small domain of size ~3x body length, with and without AMR

    doubleJunction_domLeng0.5_dx64_amr0lev_works.inputs
    doubleJunction_domLeng0.5_dx64_amr3lev4ref_works.inputs

- Larger domain (~6x body length), on a single level also works

    doubleJunction_domLeng1_dx128_amr0lev_works.inputs
    doubleJunction_domLeng1_dx32_amr0lev_works.inputs

- Larger domain (~6x body length), any AMR seems to fail

    doubleJunction_domLeng1_dx32_amr1lev4ref_fails.inputs
    doubleJunction_domLeng1_dx64_amr1lev4ref_fails.inputs
    doubleJunction_domLeng1_dx128_amr1lev4ref_fails.inputs
    doubleJunction_domLeng1_dx128_amr3lev4ref_fails.inputs
    doubleJunction_domLeng1_dx256_amr1lev4ref_fails.inputs

- Cases without the triple junction work

    doubleElectrode_domLeng1_dx128_amr1lev4ref_works.inputs
    doubleEncased_domLeng1_dx128_amr1lev4ref_works.inputs




