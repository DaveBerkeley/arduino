
#define UNUSED(x) ((x) = (x))
#define ASSERT(x) ASSERT_TRUE(x)

void mock_setup();
void mock_teardown();

bool pins_match(int num, int start, const int *pins);

//  FIN
