#include "aiws/chunking_strategy.hpp"
#include "aiws/context_strategy.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/retrieval_strategy.hpp"

#include <iostream>
#include <memory>
#include <stdExcept>
#include <string>
#include <vector>

// Student-written M2 tests
//
// Add your own tests to this file. Your tests are part of the submitted work
// and are evaluated under Student-Written Testing & Validation.
//
// Do not modify tests/public_tests.cpp.
//
// Your tests should exercise important M2 behavior beyond the supplied public
// tests. Consider default compatibility, custom strategies, runtime dispatch,
// invalid configuration, ownership/move behavior, and component interactions.

namespace {
    int failures=0;

    void check(bool ok, const char* name){
        if(!ok){
            std::cerr << "FAIL: " << name << '\n';
            failures++;
        }
    }

    class CustomChunker final : public aiws::ChunkingStrategy{
        public:
            std::vector<aiws::Chunk> chunk(const aiws::Document& doc, std::size_t doc_order) const override{
                return {{doc.id() +"#student_chunk", doc.id(), 0, doc_order, "student chunk content", 3, 0, doc.text().size()}};
            
    }
};

class CustomRetriever final : public aiws::RetrievalStrategy{
    public:
        std::vector<aiws::SearchResult> search(const std::string&, int k, const std::vector<aiws::Chunk>& chunks, const aiws::CorpusIndex&) const override {
        if(k<=0|| chunks.empty()){
            return {};
        }
        const auto& c= chunks.front();
        return {{c.id, c.document_id, c.sequence, c.text, 42.42, 1}};
    }
        
};

class CustomContext final : public aiws::ContextStrategy{
    public:
        std::vector<aiws::ContextItem> build(const std::vector<aiws::SearchResult>& ranked, std::size_t budget) const override{
            if(ranked.empty() || budget==0){
                return {};
            }
            const auto& r =ranked.front();
            return {{r.chunk_id, r.document_id, r.chunk_sequence, "student_context", 1, r.score, false}};
        }
};

}

int main() {
    // TODO: Add your own M2 tests here.
    using namespace aiws;

    {
        Workspace ws; //M1 compatability tests
        ws.add_document(Document{"doc1", "", "Vector search engines process chunks."});
        ProcessingCore core;
        core.rebuild(ws);

        check(core.chunk_count()==1, "Default constructor preserves M1 chunking");
        auto results = core.search("search",1);
        check(results.size()==1 && results[0].document_id=="doc1", "Default construcotr preserves m1 retrieval ranking");
    }

    {
        Workspace ws; //custom strat injection and dispatch
        ws.add_document(Document{"doc1", "", "some filler doc content."});

        ProcessingCore custom_core(std::make_unique<CustomChunker>(), std::make_unique<CustomRetriever>(), std::make_unique<CustomContext>());
        custom_core.rebuild(ws);

        check(custom_core.chunks()[0].id=="doc1#student_chunk", "runtime dispach calls custom chunking strat");

        auto search_result = custom_core.search("query", 1);
        check(search_result.size()==1 && search_result[0].score==42.42, "runtime dispatch calls custom retrieval strategy");

        auto ctx_result = custom_core.build_context("query", 1, 10);
        check(ctx_result.size()==1 && ctx_result[0].text=="student_context", "runtime dispatch invokes custom context strat");
    }

    {//rejection of null strat configs
        bool chunker_null_threw = false;

        try{
            ProcessingCore bad(nullptr, std::make_unique<CustomRetriever>(), std::make_unique<CustomContext>());
        } 
        catch(const std::invalid_argument&){
            chunker_null_threw=true;
        }
        check(chunker_null_threw, "null chunking strat throw invalid argument");

        bool retrieval_null_threw=false;
        try{
            ProcessingCore bad(std::make_unique<CustomChunker>(), nullptr, std::make_unique<CustomContext>());
        }
        catch(const std::invalid_argument&){
            retrieval_null_threw = true;
        }
        check(retrieval_null_threw, "null retrieval strat throws invalid argument");
        
        bool context_null_threw=false;
        try{
            ProcessingCore bad(std::make_unique<CustomChunker>(), std::make_unique<CustomRetriever>(), nullptr);
        }
        catch (const std::invalid_argument&){
            context_null_threw=true;
        }
        check(context_null_threw, "null context strat throws invalid argument");
    }

    {
        Workspace ws;
        ws.add_document(Document{"doc1", "", "move ownersjip test text."});

        ProcessingCore src_core(std::make_unique<CustomChunker>(), std::make_unique<CustomRetriever>(), std::make_unique<CustomContext>());
        src_core.rebuild(ws);

        ProcessingCore moved_core(std::move(src_core));
        check(moved_core.chunk_count()==1, "Move construction transfersindex and chunk state");
        auto search_result = moved_core.search("test", 1);
        check(search_result.size()==1 && search_result[0].score==42.42, "move construction retains custom strat");

        ProcessingCore assigned_core;
        assigned_core = std::move(moved_core);
        check(assigned_core.chunk_count()==1, "move assignment transfers ownership");
        auto ctx_result = assigned_core.build_context("test",1,10);
        check(ctx_result.size()==1 && ctx_result[0].text == "student_context", "Move assignment keeps custom strat");
    }

    {
        Workspace ws1;
        ws1.add_document(Document{"d1", "", "Initial document."});
        ProcessingCore core(std::make_unique<CustomChunker>(), std::make_unique<CustomRetriever>(), std::make_unique<CustomContext>());

        core.rebuild(ws1);
        check(core.chunks()[0].id =="d1#student_chunk", "first rebuild succeeds");

        Workspace ws2;
        ws2.add_document(Document{"d2", "", "Updated doc."});

        core.rebuild(ws2);
        check(core.chunk_count()==1 && core.chunks()[0].id=="d2#student_chunk", "Rebuild correctly replaces index while keeping injected strat");
    }
    
    if(failures){
        std::cerr << failures << "student test(s) failed.\n";
        return 1;
    }

    std:: cout << "all m2 student tests passed successfully!\n";
    return 0;
}
