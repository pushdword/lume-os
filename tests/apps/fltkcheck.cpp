// SPDX-License-Identifier: BSD-3-Clause
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Browser.H>
#include <stdio.h>

static void finish(void *window)
{
    static_cast<Fl_Window *>(window)->hide();
}

int main(int argc, char **argv)
{
    Fl_Window window(600, 400, "Lume: aplicações");
    Fl_Box title(20, 15, 560, 35, "Ficheiros, rede e navegação — ação, café, avô");
    Fl_Input input(100, 65, 460, 30, "Endereço:");
    input.value("https://dillo-browser.org/");
    Fl_File_Browser files(20, 115, 560, 220);
    int entries = files.load("/data");
    Fl_Button button(430, 350, 150, 30, "Fechar");
    button.callback(
        [](Fl_Widget *, void *p)
        {
            finish(p);
        },
        &window);
    window.end();
    window.resizable(files);
    window.show();
    if (argc > 1)
        Fl::add_timeout(3, finish, &window);
    printf("FLTK_READY entries=%d\n", entries);
    fflush(stdout);
    int result = Fl::run();
    printf("FLTK_EXIT %d\n", result);
    return result;
}
