// SPDX-License-Identifier: BSD-3-Clause
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "lume/net_abi.h"
#include "lume/service.h"
extern "C"
{
#include "../../services/netd/net_policy.h"
}
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

class Network
{
    Fl_Double_Window window{720, 420, "Rede"};
    Fl_Box interfaces{10, 10, 510, 65};
    Fl_Button ethernet_button{535, 15, 175, 30, "Ligar Ethernet"};
    Fl_Browser profiles{10, 85, 205, 125, "Perfis Wi-Fi"};
    Fl_Button connect{10, 220, 95, 28, "Ligar"};
    Fl_Button pause{115, 220, 100, 28, "Desligar"};
    Fl_Check_Button automatic{10, 255, 205, 28, "Ligação automática"};
    Fl_Button reconnect{10, 290, 205, 28, "Voltar a ligar"};
    Fl_Browser networks{235, 85, 475, 125, "Redes encontradas"};
    Fl_Button scan{235, 220, 120, 28, "Pesquisar"};
    Fl_Input name{440, 220, 270, 28, "Perfil:"};
    Fl_Input ssid{330, 255, 380, 28, "SSID:"};
    Fl_Secret_Input key{330, 290, 380, 28, "Palavra-passe:"};
    Fl_Button save{490, 330, 220, 30, "Guardar perfil WPA2"};
    Fl_Box status{10, 365, 700, 45};
    lume_net_service_status service{};
    lume_wifi_bss_info found[LUME_NET_SCAN_MAX]{};
    unsigned found_count = 0, wifi = 0, ethernet = 0;
    bool ethernet_up = false;
    uint64_t sequence = 0, deadline = 0;
    char staged[64] = "";
    char message[1024] = "";

    void feedback(const char *text)
    {
        status.copy_label(text);
    }

    static uint64_t now()
    {
        return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
    }

    void send(unsigned command, const char *profile = nullptr)
    {
        if (sequence)
        {
            feedback("A aguardar a operação anterior.");
            return;
        }
        lume_service_message request{};
        request.version = 1;
        request.command = command;
        if (profile)
        {
            if (!net_name(profile))
            {
                feedback("Nome de perfil inválido.");
                return;
            }
            snprintf(request.name, sizeof(request.name), "%s", profile);
        }
        int64_t result = lume_native_call(LUME_SYS_SERVICE, LUME_SERVICE_SEND, (uintptr_t)&request,
                                          sizeof(request));
        if (result < 0)
        {
            if (*staged)
            {
                unlink(staged);
                *staged = 0;
            }
            feedback("Serviço ocupado ou permissões insuficientes.");
            return;
        }
        sequence = (uint64_t)result;
        deadline = now() + 10000;
        feedback("A aplicar configuração...");
    }

    void refresh()
    {
        lume_net_control request{};
        request.version = LUME_NET_CONTROL_VERSION;
        if (lume_native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, (uintptr_t)&request, sizeof(request)) <
            0)
            interfaces.copy_label("Interfaces indisponíveis.");
        else
        {
            wifi = ethernet = 0;
            size_t used = 0;
            *message = 0;
            for (unsigned i = 0; i < request.count && i < LUME_NET_INTERFACES; ++i)
            {
                const lume_netif_info &info = request.data.interfaces[i];
                if (info.kind == LUME_IF_WIFI)
                    wifi = info.ifindex;
                if (info.kind == LUME_IF_ETHERNET)
                {
                    ethernet = info.ifindex;
                    ethernet_up = (info.flags & LUME_IF_UP) != 0;
                }
                int count =
                    snprintf(message + used, sizeof(message) - used, "%.16s: %s  %u.%u.%u.%u\n",
                             info.name, info.flags & LUME_IF_LINK ? "ligada" : "sem ligação",
                             info.address[0], info.address[1], info.address[2], info.address[3]);
                if (count < 0 || (size_t)count >= sizeof(message) - used)
                    break;
                used += (size_t)count;
            }
            interfaces.copy_label(*message ? message : "Nenhuma interface de rede.");
            ethernet_button.copy_label(ethernet_up ? "Desligar Ethernet" : "Ligar Ethernet");
        }
        lume_net_service_status next{};
        if (lume_native_call(LUME_SYS_SERVICE, LUME_SERVICE_GET, (uintptr_t)&next, sizeof(next)) <
            0)
            return;
        if (next.version != 1 || next.profiles > 4)
            return;
        if (memcmp(next.profile, service.profile, sizeof(next.profile)))
        {
            profiles.clear();
            for (unsigned i = 0; i < next.profiles; ++i)
                profiles.add(next.profile[i].name);
        }
        service = next;
        automatic.value(service.automatic != 0);
        if (sequence && (service.acknowledged == sequence || now() >= deadline))
        {
            if (*staged)
            {
                unlink(staged);
                *staged = 0;
            }
            if (service.acknowledged != sequence)
                feedback("O serviço não respondeu dentro do prazo.");
            else if (service.result)
                feedback("O serviço rejeitou a configuração.");
            else
                feedback("Configuração aplicada.");
            sequence = 0;
        }
    }

    void search()
    {
        if (!wifi)
        {
            feedback("Não existe uma interface Wi-Fi.");
            return;
        }
        lume_net_control request{};
        request.version = LUME_NET_CONTROL_VERSION;
        request.ifindex = wifi;
        if (lume_native_call(LUME_SYS_NETCTL, LUME_NET_WIFI_SCAN, (uintptr_t)&request,
                             sizeof(request)) < 0)
        {
            feedback("A pesquisa Wi-Fi falhou.");
            return;
        }
        found_count = request.count > LUME_NET_SCAN_MAX ? LUME_NET_SCAN_MAX : request.count;
        memcpy(found, request.data.scan, found_count * sizeof(found[0]));
        networks.clear();
        for (unsigned i = 0; i < found_count; ++i)
        {
            char text[100];
            unsigned bytes = found[i].ssid_bytes > 32 ? 32 : found[i].ssid_bytes;
            snprintf(text, sizeof(text), " %.*s  (%d dBm)", (int)bytes, found[i].ssid,
                     found[i].rssi);
            networks.add(text);
        }
        feedback("Selecione uma rede e indique a palavra-passe WPA2.");
    }

    void save_profile()
    {
        if (sequence)
            return;
        const char *profile = name.value(), *network = ssid.value(), *password = key.value();
        size_t network_bytes = strlen(network), key_bytes = strlen(password);
        if (!net_import_path(profile, staged, sizeof(staged)) || !network_bytes ||
            network_bytes > 32 || key_bytes < 8 || key_bytes > 63)
        {
            *staged = 0;
            feedback(
                "Perfil: até 15 letras, dígitos, _ ou -. SSID: até 32 bytes. Chave: 8 a 63 caracteres.");
            return;
        }
        for (size_t i = 0; i < key_bytes; ++i)
            if ((unsigned char)password[i] < 32 || (unsigned char)password[i] > 126)
            {
                *staged = 0;
                feedback("A chave WPA2 deve conter caracteres ASCII imprimíveis.");
                return;
            }
        unsigned char encoded[104] = {'L', 'W', 'N', '1'};
        encoded[4] = (unsigned char)network_bytes;
        encoded[5] = (unsigned char)key_bytes;
        memcpy(encoded + 8, network, network_bytes);
        memcpy(encoded + 40, password, key_bytes);
        if (mkdir("/data/system", 0700) && errno != EEXIST)
        {
            net_wipe(encoded, sizeof(encoded));
            key.value("");
            *staged = 0;
            feedback("A pasta de configuração está indisponível.");
            return;
        }
        int fd = open(staged, O_WRONLY | O_CREAT | O_EXCL, 0600);
        bool ok = fd >= 0;
        size_t done = 0;
        while (ok && done < sizeof(encoded))
        {
            ssize_t count = write(fd, encoded + done, sizeof(encoded) - done);
            if (count <= 0)
                ok = false;
            else
                done += (size_t)count;
        }
        net_wipe(encoded, sizeof(encoded));
        key.value("");
        if (fd >= 0 && close(fd))
            ok = false;
        if (!ok)
        {
            if (fd >= 0)
                unlink(staged);
            *staged = 0;
            feedback("Não foi possível guardar o perfil temporário. Verifique se já existe.");
            return;
        }
        send(NETD_IMPORT_FILE, profile);
    }

    static void timer(void *context)
    {
        auto network = static_cast<Network *>(context);
        network->refresh();
        Fl::repeat_timeout(1, timer, context);
    }

  public:
    Network()
    {
        window.end();
        window.resizable(status);
        interfaces.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        status.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        profiles.type(FL_HOLD_BROWSER);
        networks.type(FL_HOLD_BROWSER);
        name.value("wifi");
        scan.callback(
            [](Fl_Widget *, void *p)
            {
                static_cast<Network *>(p)->search();
            },
            this);
        save.callback(
            [](Fl_Widget *, void *p)
            {
                static_cast<Network *>(p)->save_profile();
            },
            this);
        connect.callback(
            [](Fl_Widget *, void *p)
            {
                auto network = static_cast<Network *>(p);
                network->send(NETD_CONNECT, network->profiles.text(network->profiles.value()));
            },
            this);
        pause.callback(
            [](Fl_Widget *, void *p)
            {
                static_cast<Network *>(p)->send(NETD_PAUSE);
            },
            this);
        reconnect.callback(
            [](Fl_Widget *, void *p)
            {
                static_cast<Network *>(p)->send(NETD_RECONNECT);
            },
            this);
        ethernet_button.callback(
            [](Fl_Widget *, void *p)
            {
                auto network = static_cast<Network *>(p);
                if (!network->ethernet)
                {
                    network->feedback("Não existe uma interface Ethernet.");
                    return;
                }
                lume_net_control request{};
                request.version = LUME_NET_CONTROL_VERSION;
                request.ifindex = network->ethernet;
                request.data.admin_up = !network->ethernet_up;
                int64_t result = lume_native_call(LUME_SYS_NETCTL, LUME_NET_ADMIN,
                                                  (uintptr_t)&request, sizeof(request));
                network->feedback(result < 0 ? "Não foi possível alterar a interface."
                                             : "Interface atualizada.");
                network->refresh();
            },
            this);
        automatic.callback(
            [](Fl_Widget *, void *p)
            {
                auto network = static_cast<Network *>(p);
                network->send(network->automatic.value() ? NETD_AUTO_ON : NETD_AUTO_OFF);
            },
            this);
        networks.callback(
            [](Fl_Widget *, void *p)
            {
                auto network = static_cast<Network *>(p);
                unsigned selected = (unsigned)network->networks.value();
                if (selected && selected <= network->found_count)
                {
                    const lume_wifi_bss_info &entry = network->found[selected - 1];
                    char text[33];
                    unsigned bytes = entry.ssid_bytes > 32 ? 32 : entry.ssid_bytes;
                    memcpy(text, entry.ssid, bytes);
                    text[bytes] = 0;
                    network->ssid.value(text);
                }
            },
            this);
        refresh();
        Fl::add_timeout(1, timer, this);
        window.show();
    }

    ~Network()
    {
        Fl::remove_timeout(timer, this);
        if (*staged && !sequence)
            unlink(staged);
    }
};

int main()
{
    Network network;
    return Fl::run();
}
