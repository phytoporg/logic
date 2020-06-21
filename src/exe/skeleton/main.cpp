#include <logik/logik.h>

int main()
{
    logik::Instance instance("Skeleton");

    logik::WindowPtr spWindow = instance.CreateWindow(600, 400, "skeleton");
    spWindow->PollEvents();

    return 0;
}

