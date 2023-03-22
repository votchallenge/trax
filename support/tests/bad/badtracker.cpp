#include <trax.h>
#include <signal.h>
#include <stdlib.h>

int main( int argc, char** argv) {

  trax::Server handle(trax::Metadata(TRAX_REGION_RECTANGLE, TRAX_IMAGE_ANY), trax_no_log);

  int timetobreak = (rand() % 10) + 1;
  int breaktype = 1;

  trax::ObjectList memory;

  while (true) {

    trax::ImageList image;
    trax::ObjectList objects;
    trax::Properties properties;

    int tr = handle.wait(image, objects, properties);

    fprintf(stderr, "Response %d \n", tr);
    if (tr == TRAX_INITIALIZE) {

      timetobreak = properties.get("break_time", 5);
      std::string type = properties.get("break_type", "sigabrt");
      if (type == "sigsegv")
        breaktype = 2;
      else if (type == "sigusr1")
        breaktype = 3;
      else
        breaktype = 1;

      handle.reply(objects);

      memory = objects;
      fprintf(stderr, "Reply %d \n", memory.size());
    } else if (tr == TRAX_FRAME) {
      fprintf(stderr, "Reply %d \n", memory.size());
      handle.reply(memory);

    }
    else break;

    timetobreak--;

    fprintf(stderr, "Countdown %d \n", timetobreak);

    if (timetobreak <= 0) {
      switch (breaktype) {
      case 1:
        raise (SIGABRT);
      case 2:
        raise (SIGSEGV);
#ifndef _MSC_VER
      case 3:
        raise (SIGUSR1);
#endif
      default:
        abort();
      }
    }

  }

}

