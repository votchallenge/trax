#include <trax.h>
#include <signal.h>
#include <stdlib.h>

int main( int argc, char** argv) {

  trax::Server handle(trax::Metadata(TRAX_REGION_RECTANGLE, TRAX_IMAGE_ANY), trax_no_log);

  int timetobreak = (rand() % 10) + 1;
  int breaktype = 1;

  trax::Region memory;

  while (true) {

    trax::ImageList image;
    trax::Region region;
    trax::Properties properties;

    int tr = handle.wait(image, region, properties);
    if (tr == TRAX_INITIALIZE) {

      timetobreak = properties.get("break_time", 5);

      std::string type = properties.get("break_type", "sigabrt");
      if (type == "sigsegv")
        breaktype = 2;
      else if (type == "sigusr1")
        breaktype = 3;
      else
        breaktype = 1;

      handle.reply(region, trax::Properties());

      memory = region;

    } else if (tr == TRAX_FRAME) {

      handle.reply(memory, trax::Properties());

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

