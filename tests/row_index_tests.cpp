#include "core/alignment.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <algorithm>
// Exercise every allocating preflight position, including index-node allocation.
// The allocator is armed only around one document command, never Qt or logging.
namespace { long allocationsBeforeFailure=-1; }
void* operator new(std::size_t size){
    if(allocationsBeforeFailure==0)throw std::bad_alloc{};
    if(allocationsBeforeFailure>0)--allocationsBeforeFailure;
    if(auto* result=std::malloc(size?size:1))return result;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::string snapshot(const Document& d){
    std::ostringstream out;out<<d.state_id()<<':'<<d.content_id()<<':'<<d.next_sequence_id()<<':'<<d.can_undo()<<':'<<d.can_redo()<<'\n';
    for(const auto& row:d.rows())out<<row.id<<':'<<row.name<<':'<<row.symbols.text()<<'\n';
    return out.str();
}
void index_valid(const Document& d){
    allocationsBeforeFailure=0;
    for(const auto& row:d.rows())if(d.find_sequence(row.id)!=&row){allocationsBeforeFailure=-1;throw std::runtime_error("Index points to a different row");}
    const bool missing=d.find_sequence(d.next_sequence_id())==nullptr;
    allocationsBeforeFailure=-1;need(missing,"Absent ID is not indexed");
}
Document baseline(){
    Document d;d.load({{"first","ACGT"},{"second","ACGT"},{"third","GATTACA"}});
    d.rename({{d.rows()[0].id,"renamed"}});d.undo();return d;
}
void failure_positions(const char* label,const std::function<Document()>& setup,const std::function<void(Document&)>& change){
    std::size_t refused=0;
    for(long point=0;point<512;++point){
        auto d=setup();const auto before=snapshot(d);
        try{allocationsBeforeFailure=point;change(d);allocationsBeforeFailure=-1;index_valid(d);need(refused>0,"No allocation failure exercised");std::cout<<"PASS "<<label<<": "<<refused<<" allocation failures retained document/history\n";return;}
        catch(const std::bad_alloc&){allocationsBeforeFailure=-1;++refused;need(snapshot(d)==before,"Refused command changed rows, IDs or history");index_valid(d);}
        catch(...){allocationsBeforeFailure=-1;throw;}
    }
    throw std::runtime_error("Allocation failure sweep did not reach a complete command");
}
}
int main(){try{
    failure_positions("append",baseline,[](auto& d){d.append({{"new","TTTT"},{"another","CCCC"}});});
    failure_positions("erase multiple spans",baseline,[](auto& d){d.erase_rows({d.rows()[0].id,d.rows()[2].id});});
    failure_positions("reorder",baseline,[](auto& d){d.reorder({d.rows()[2].id,d.rows()[0].id,d.rows()[1].id});});
    failure_positions("load baseline",baseline,[](auto& d){d.load({{"replacement","ATAT"}});});
    failure_positions("restore session",baseline,[](auto& d){d.restore_session({{"restored","ATGC"}},{901},1000);});
    failure_positions("composite collapse",baseline,[](auto& d){d.collapse_identical({{d.rows()[0].id,{d.rows()[0].id,d.rows()[1].id},"combined"}});});
    failure_positions("undo splice",[]{auto d=baseline();d.append({{"new","T"}});return d;},[](auto& d){d.undo();});
    failure_positions("redo splice",[]{auto d=baseline();d.append({{"new","T"}});d.undo();return d;},[](auto& d){d.redo();});
    Document d=baseline();
    for(int i=0;i<200;++i){
        d.append({{"cycle","ACGT"}});const auto id=d.rows().back().id;index_valid(d);
        auto order=std::vector<SequenceId>{};for(const auto& r:d.rows())order.push_back(r.id);std::reverse(order.begin(),order.end());d.reorder(order);index_valid(d);
        d.undo();index_valid(d);d.redo();index_valid(d);d.erase_rows({id});index_valid(d);d.undo();index_valid(d);d.undo();d.undo();index_valid(d);
    }
    std::cout<<"PASS structural cycles and allocation-free const lookup\n";return 0;
}catch(const std::exception& e){allocationsBeforeFailure=-1;std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
