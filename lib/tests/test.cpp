
#include <trax.h>

using namespace trax;
using namespace std;

int main( int argc, char** argv) {

	cout << "Testing reference counting for region wrapper." << endl;

	{

		Region r1;
		Region r2 = Region::create_rectangle(10, 10, 10, 10);

		{
			Region r4 = r1;
			Region r3 = r2;
			r1 = r3;
		}

	}

	cout << "Testing reference counting for image wrapper." << endl;

	{

		Image i1;
		Image i2 = Image::create_path("/tmp/test.jpg");

		{
			Image i4 = i1;
			Image i3 = i2;
			i1 = i3;
		}

	}

	cout << "Testing reference counting and allocation for properties wrapper." << endl;

	{

		Properties p1;
		Properties p2;
		p1.set("key1", "a");
		p1.set("key2", "b");

		p2 = p1;

		p2.set("key3", "c");
		p2.set("key2", "c");

		cout << p1 << endl;
		cout << p2 << endl;

		p1.clear();

		cout << p1 << endl;
		cout << p2 << endl;		
	}



    return 0;
}



