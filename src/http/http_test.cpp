#include <http_client.h>
#include <errors.h>

using namespace pp;

static int test_Get(http::ClientRef &client)
{
  errors::Error			err;

	auto resp = client->Get("http://www.baidu.com", err);
	if (resp == nullptr){
		printf(" err: %s\n", err.what());
		return -1;
	}
	printf("Url : %s\n", resp->Request_->Url.c_str());

	for (auto it = resp->Header.begin(); it != resp->Header.end(); it++){
		printf("%s : %s\n", it->first.c_str(), it->second.c_str());
	}
	printf("content:%s\n", resp->Content.c_str());
	return 0;
}

static int test_Post(http::ClientRef &client)
{
  errors::Error			err;

	auto resp = client->Post("http://www.example.com",
                           http::ContentTypeJson,
                           "{\"key\" : \"value\"}", err);
	if (resp == nullptr){
		printf(" err: %s\n", err.what());
		return -1;
	}
	printf("Url : %s\n", resp->Request_->Url.c_str());

	for (auto it = resp->Header.begin(); it != resp->Header.end(); it++){
		printf("%s : %s\n", it->first.c_str(), it->second.c_str());
	}
	printf("content:%s\n", resp->Content.c_str());

	return 0;
}


//please run it with commond line
int main()
{

	auto httpClient = http::NewClient();

	printf("*********************GET********************\n");
	if (test_Get(httpClient) != 0){
    return 1;
  }

	printf("********************POST*******************\n");
	if(test_Post(httpClient) != 0){
    return 1;
  }
  return 0;
}
