# M2 Technical & Design Understanding

This file is an assessed technical-understanding artifact, not ordinary project documentation.
Answer all four questions using your own submitted implementation. Concise answers are acceptable when they are technically correct and specific.

Generic descriptions of C++ concepts or restatements of the assignment that do not identify and explain corresponding parts of your code will receive limited credit.

## 1. Polymorphism and dynamic dispatch - 1.5 points

Identify one place in your M2 implementation where runtime polymorphism occurs. Name the relevant base interface, derived implementation, and `ProcessingCore` function involved. Trace the call from `ProcessingCore` to the selected strategy implementation and explain why the derived implementation is invoked.

Then explain what would change if the relevant operation were not declared `virtual`.

Runtime polymorphism takes place in Processing Core::rebuild in processing_core.cpp when the system generates document chunks. The base interface here is ChunkingStrategy and the coordinating function is ProcessingCore::rebuild. ProcessingCore::rebuild calls produced= impl_->chunker->chunk(doc,order). Since imple_->chunker is a unique_ptr<ChunkingStrategy> holding a derived instance on the heap, c++ resolves the call at runtime using the objects virtual method table. The runtime looks up chunk() inside the table and dispatches directly to the method. If it were not declared virtual in ChunkingStrategy, c++ would fall back to compile time static binding based on the chunking strategy type. As a result, calling chunk() would either trigger a build error if pure virtual or execute the base class logic, ignoring any overrides.

## 2. Ownership and lifetime - 1.5 points

Identify where one of the strategy objects is created, where ownership is transferred, and which object ultimately owns it. Explain how `std::unique_ptr` represents that ownership relationship and when the strategy object is destroyed.

Also explain why `ProcessingCore` is move-only and why the strategy base classes require virtual destructors.

Strategy creation and ownership transfer happens in tests/student_tests.cpp, where std::make_unique<CustomeRetriever>() creates a strategy on the heap and passes it by value into ProcessingCore's configurable constructor. Inside processing_core.cpp, ProcessingCore verifies that the incoming strategy pointers aren't null and moves exclusive ownership into its internal plmpl struct using imple_ = std::make_unique<Impl>(std::move(chunking), std::move(retrieval), std::move(context)). std::unique_ptr enforces single ownership semantics through move only lifetime tracking. When ProcessingCore goes out of scope, its distructor cleans up Impl which automatically triggers ~unique_ptr() on each strategy and safely frees the underlying heap memory by RAII. ProcessingCore is move only because unique_ptr can't be copied. Allowing copies would cause duplicate ownership claims or dangling pointers, whereas move operations keep ownership transfers explicit and safe. Base strategy classes also require virtual ~Interface()=default to avoid undefined behavior during deletion. Deleting a strategy through a base pointer also ensures the derived destructor runs first and properly frees allocations before the base class cleans up.

## 3. Architecture, extensibility, and M1 compatibility - 1.5 points

Explain one specific architectural decision in your M2 implementation that makes the processing system extensible while preserving M1 behavior.

Identify the classes or interfaces involved and explain both:
- how the default configuration preserves M1 behavior; and
- how a different implementation can be substituted without changing the normal `ProcessingCore` API.

Include one plausible design alternative and explain why the M2 design is preferable for this milestone. The alternative does not need to be something you actually implemented.

The core architectural decision in M2 is using constructor based dependency injection paired with smart pointers and the Pimpl pattern. Instead of hardcoding logic, ProcessingCore delegates work to ChunkingStrategy, RetrievalStrategy, and ContextStrategy abstractions stored as unique pointers inside ProcessingCore::Impl. When calling the default constructor for ProcessingCore, Impl is instantiated using default m1 implementations. These default classes retain the same m1 algorithms. Running rebuild(), search(), or build_context() returns the same results as m1. Using a different strategy, a caller derives from a base interface like RetrievalStrategy and passes a unique_ptr<CustomRetriever> into the configurable constructor. Since ProcessingCore::search delegates directly to impl_retrieval->search(), dynamic dispatch invokes the custom strategy without changing ProcessingCore's API signatrue. An alternative design could use a global Registry. The dependency injection approach is better because it eliminates global mutable state, relies on RAII for lifetime management, and catches invalid configurations at construction time rather than through runtime lookups.

## 4. Testing and defect reasoning - 1.5 points

Select one meaningful test from your `tests/student_tests.cpp`.

Explain:
- what M2 requirement the test validates;
- what specific implementation defect the test could detect; and
- why your test provides useful evidence beyond simply rerunning the supplied public tests.

If your test uses a custom strategy, explain how its observable behavior demonstrates that `ProcessingCore` is actually using runtime substitution.

In my tests, the second test tests custom strategy injection and dynamic dispatch. This test validates m2 section 7 by ensuring custom strategies passed into ProcessingCore actually override processing during rebuild(), search(), and build_context(). In my code ProcessingCore custom_core() is constructed with CustomChunker, CustomRetriever, and CustomContext. The test then verifies that custom_core.chunks()[0].id is equal to doc1#student_ and custom_core.search("query", 1)[0].score is equal to 42.42. This test catches defects where ProcessingCore might store strategy pointers but still secretly invoke hardcoded m1 concrete members or bypass dynamic dispatch entirely. It provides valuable evidence beyond the public tests because it uses distinct marker ids and fixed sentinel values that the default algorithms could not generate. Seeing 42.42 come back proves that ProcessingCore dispatched through the virtual stratefy interface instead of falling back on tf-idf scoring.