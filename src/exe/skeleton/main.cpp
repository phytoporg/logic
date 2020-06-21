#include <logik/logik.h>

int main()
{
    logik::Instance instance("Skeleton");

    logik::WindowPtr spWindow = instance.CreateWindow(640, 480, "skeleton");
    spWindow->PollEvents();

    return 0;
}

