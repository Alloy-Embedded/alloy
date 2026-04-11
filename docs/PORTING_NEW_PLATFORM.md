# Porting a New Platform

## Goal

A new platform should add as little handwritten runtime logic as possible.

Before adding a platform:

- make sure `alloy-codegen` and `alloy-devices` already publish a typed descriptor contract for that family
- verify whether the platform fits an existing runtime backend schema

## Preferred Order

1. Extend `alloy-codegen` and `alloy-devices` first.
2. Reuse an existing runtime schema backend if possible.
3. Only add runtime code for genuinely new hardware semantics.
4. Add compile smoke and board coverage.

## Avoid

- family-specific switches spread across the runtime
- handwritten register offsets already available in generated descriptors
- string parsing in the runtime when typed refs exist

## References

- [ARCHITECTURE.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)
- [CODE_GENERATION.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
