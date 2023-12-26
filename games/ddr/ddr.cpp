#include "ddr.h"

#include "acioemu/handle.h"
#include "avs/game.h"
#include "hooks/devicehook.h"
#include "hooks/setupapihook.h"
#include "hooks/sleephook.h"
#include "hooks/input/dinput8/hook.h"
#include "util/utils.h"
#include "util/libutils.h"
#include "util/fileutils.h"
#include "util/detour.h"

#include "io.h"

#include "p3io/foot.h"
#include "p3io/p3io.h"
#include "p3io/sate.h"
#include "p3io/usbmem.h"

using namespace acioemu;

namespace games::ddr {

    // settings
    bool SDMODE = false;

    static decltype(SendMessage) *SendMessage_real = nullptr;

    static SHORT WINAPI GetAsyncKeyState_hook(int vKey) {

        // disable debug keys
        return 0;
    }

    static LRESULT WINAPI SendMessage_hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

        // ignore broadcasts
        if (hWnd == HWND_BROADCAST) {
            return 1;
        }

        // fallback
        return SendMessage_real(hWnd, Msg, wParam, lParam);
    }

    DDRGame::DDRGame() : Game("Dance Dance Revolution") {
    }

    void DDRGame::attach() {
        Game::attach();

        // dinput hook on this dll since the game dll doesn't load it
        auto game_mdx = libutils::try_library(MODULE_PATH / "gamemdx.dll");
        hooks::input::dinput8::init(game_mdx);

        // init device hook
        devicehook_init();

        // add fake devices
        if (avs::game::DLL_NAME == "arkmdxbio2.dll") {
            devicehook_add(new acioemu::ACIOHandle(L"COM1"));
        } else {
            devicehook_add(new DDRFOOTHandle());
            devicehook_add(new DDRSATEHandle());
            devicehook_add(new DDRUSBMEMHandle());
            devicehook_add(new DDRP3IOHandle());
        }

        // has nothing to do with P3IO, but is enough to trick the game into SD/HD mode
        const char *settings_property = ddr::SDMODE ? "Generic Television" : "Generic Monitor";
        const char settings_detail[] = R"(\\.\P3IO)";

        // settings 1
        SETUPAPI_SETTINGS settings1 {};
        settings1.class_guid[0] = 0x1FA4A480;
        settings1.class_guid[1] = 0x40C7AC60;
        settings1.class_guid[2] = 0x7952ACA7;
        settings1.class_guid[3] = 0x5A57340F;
        memcpy(settings1.property_devicedesc, settings_property, strlen(settings_property) + 1);
        memcpy(settings1.interface_detail, settings_detail, sizeof(settings_detail));

        // settings 2
        SETUPAPI_SETTINGS settings2 {};
        settings2.class_guid[0] = 0x4D36E96E;
        settings2.class_guid[1] = 0x11CEE325;
        settings2.class_guid[2] = 0x8C1BF;
        settings2.class_guid[3] = 0x1803E12B;
        memcpy(settings2.property_devicedesc, settings_property, strlen(settings_property) + 1);
        memcpy(settings2.interface_detail, settings_detail, sizeof(settings_detail));

        // init SETUP API
        setupapihook_init(avs::game::DLL_INSTANCE);

        // DDR ACE actually uses another DLL for things
        if (game_mdx != nullptr) {
            setupapihook_init(game_mdx);
        }

        // add settings
        setupapihook_add(settings1);
        setupapihook_add(settings2);

        // misc hooks
        detour::iat_try("GetAsyncKeyState", GetAsyncKeyState_hook, avs::game::DLL_INSTANCE);
        detour::iat_try("GetKeyState", GetAsyncKeyState_hook, avs::game::DLL_INSTANCE);
        SendMessage_real = detour::iat_try("SendMessageW", SendMessage_hook, avs::game::DLL_INSTANCE);
        detour::iat_try("SendMessageA", SendMessage_hook, avs::game::DLL_INSTANCE);
    }

    void DDRGame::detach() {
        Game::detach();

        // dispose device hook
        devicehook_dispose();
    }
}
