#include <ServiceFactory.h>
#include <ServiceRegistry.h>

namespace itc
{
  const std::shared_ptr<CurrentLogType>& getLog();
}



using json = nlohmann::json;

json exclude_policy=json::parse("[]");

typedef itc::sys::CancelableThread<LAppS::abstract::Service> ServiceInstanceType;

int main()
{
  LAppS::ServiceRegistry aRegistry;

  auto serverThread=LAppS::ServiceFactory::get(LAppS::ServiceProtocol::RAW,LAppS::ServiceLanguage::LUA,"echo","/echo", 2048, LAppS::Network_ACL_Policy::ALLOW,exclude_policy);
  aRegistry.reg(serverThread);
  auto clientThread=LAppS::ServiceFactory::get(LAppS::ServiceLanguage::LUA,"benchmark");
  aRegistry.reg(clientThread);

  try{
    if(aRegistry.findById(serverThread->getRunnable()->getInstanceId())->getInstanceId() != serverThread->getRunnable()->getInstanceId())
    {
      std::cout << "Test 1 (findByInstanceId) is failed" << std::endl;
    }
    else std::cout << "Test 1 passed" << std::endl;

    if(aRegistry.findById(clientThread->getRunnable()->getInstanceId())->getInstanceId() != clientThread->getRunnable()->getInstanceId())
    {
      std::cout << "Test 2 (findByInstanceId) is failed" << std::endl;
    }
    else
    {
      std::cout << "Test 2 passed" << std::endl;
    }
    if(aRegistry.findByTarget("/echo")->getInstanceId() != serverThread->getRunnable()->getInstanceId())
    {
      std::cout << "Test 3 (findByTarget) is failed" << std::endl;
    }
    else
    {
      std::cout << "Test 3 passed" << std::endl;
    }
    if(aRegistry.findByName("benchmark")->getInstanceId() != clientThread->getRunnable()->getInstanceId())
    {
      std::cout << "Test 4 (findByName) is failed" << std::endl;
    }
    else
    {
      std::cout << "Test 4 passed" << std::endl;
    }
  }catch(const std::exception& e)
  {
    std::cout << "Test scope [findBy...] is failed with exception: "<< e.what() << std::endl;
  }
  auto serverThread1=LAppS::ServiceFactory::get(LAppS::ServiceProtocol::RAW,LAppS::ServiceLanguage::LUA,"echo","/echo", 2048, LAppS::Network_ACL_Policy::ALLOW,exclude_policy);
  aRegistry.reg(serverThread1);

  int c=getchar();

  auto lst=aRegistry.list();

  std::cout << lst->dump(2) << std::endl;
}
