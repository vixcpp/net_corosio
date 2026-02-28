# net_corosio

net_corosio is an experimental coroutine-based networking backend for Vix.cpp.

It exists to evaluate, benchmark, and understand modern coroutine-driven networking models in C++ at runtime scale.

This repository is not a foundation of Vix.
It is a research layer.

## The Problem

Modern C++ networking is powerful, but fragmented.

You have:

- Boost.Asio
- Coroutine abstractions
- TLS integrations
- Multiple executor models
- Competing async patterns

What you do not have:

- A dominant coroutine runtime model
- A cohesive execution philosophy
- Clear architectural boundaries
- A stable networking layer for runtime systems

When building a runtime like Vix, this fragmentation becomes a strategic risk.

## Why net_corosio Exists

Before adopting any networking backend into a runtime, it must be evaluated.

Not by hype.
Not by benchmarks alone.
But by architectural impact.

net_corosio exists to answer critical questions:

- How does a coroutine-based transport behave under load?
- What are the scheduler and executor tradeoffs?
- How predictable are its failure semantics?
- How complex is TLS integration?
- How tightly coupled is the abstraction model?
- What does long-term maintainability look like?

This repository allows Vix to measure before committing.

## This Is Not a Dependency

net_corosio is not the core of Vix.

It is isolated by design.

Its role is:

- Benchmarking
- Architectural comparison
- Integration boundary validation
- Coroutine model exploration
- Performance and failure analysis

A runtime cannot afford blind coupling.

## Why This Matters

Runtimes live for years.

Networking decisions define:

- Concurrency models
- Error propagation behavior
- Observability boundaries
- Backpressure handling
- Memory characteristics
- Stability under stress

Once exposed publicly, those decisions become difficult to reverse.

net_corosio protects Vix from premature architectural lock-in.

## Philosophy

- Measure before adopting
- Isolate before coupling
- Control before convenience
- Stability over novelty
- Architecture before benchmarks

## Status

net_corosio is experimental.

It may evolve.
It may influence Vix.
It may be replaced.

Its purpose is clarity.

## Part of the Vix Ecosystem

Vix is building a modern C++ runtime with:

- Deterministic design
- Explicit boundaries
- Modular architecture
- Long-term maintainability

net_corosio contributes to that mission by ensuring networking decisions are deliberate, not accidental.

## License

MIT License

Copyright (c) 2026 Vix.cpp\
Author: Gaspard Kirira

