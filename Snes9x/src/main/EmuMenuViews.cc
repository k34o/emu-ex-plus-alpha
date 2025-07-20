#include <emuframework/EmuApp.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/DataPathSelectView.hh>
#include <emuframework/UserPathSelectView.hh>
#include <emuframework/SystemActionsView.hh>
#include <emuframework/viewUtils.hh>
#include "EmuCheatViews.hh"
#include "MainApp.hh"
#include <imagine/util/format.hh>
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#endif
#include <imagine/logger/logger.h>
//#include <emuframework/CollectTextInputView.hh>
#ifndef SNES9X_VERSION_1_4
#include <memory.h> // Snes9xのメモリアクセス用
#endif
#include "memmap.h"
#include <stdlib.h>

namespace EmuEx
{

using MainAppHelper = EmuAppHelperBase<MainApp>;

constexpr bool HAS_NSRT = !IS_SNES9X_VERSION_1_4;

#ifndef SNES9X_VERSION_1_4
class CustomAudioOptionView : public AudioOptionView, public MainAppHelper
{
	using MainAppHelper::system;

	void setDSPInterpolation(uint8_t val)
	{
		logMsg("set DSP interpolation:%u", val);
		system().optionAudioDSPInterpolation = val;
		SNES::dsp.spc_dsp.interpolation = val;
	}

	TextMenuItem dspInterpolationItem[5]
	{
		{"None",     attachParams(), [this](){ setDSPInterpolation(0); }},
		{"Linear",   attachParams(), [this](){ setDSPInterpolation(1); }},
		{"Gaussian", attachParams(), [this](){ setDSPInterpolation(2); }},
		{"Cubic",    attachParams(), [this](){ setDSPInterpolation(3); }},
		{"Sinc",     attachParams(), [this](){ setDSPInterpolation(4); }},
	};

	MultiChoiceMenuItem dspInterpolation
	{
		"DSP Interpolation", attachParams(),
		system().optionAudioDSPInterpolation.value(),
		dspInterpolationItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach, EmuAudio &audio): AudioOptionView{attach, audio, true}
	{
		loadStockItems();
		item.emplace_back(&dspInterpolation);
	}
};
#endif

class ConsoleOptionView : public TableView, public MainAppHelper
{
	BoolMenuItem multitap
	{
		"5-Player Adapter", attachParams(),
		(bool)system().optionMultitap,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionMultitap = item.flipBoolValue(*this);
			system().setupSNESInput(app().defaultVController());
		}
	};

	TextMenuItem inputPortsItem[HAS_NSRT ? 5 : 4]
	{
		#ifndef SNES9X_VERSION_1_4
		{"Auto (NSRT)", attachParams(), setInputPortsDel(), {.id = SNES_AUTO_INPUT}},
		#endif
		{"Gamepads",    attachParams(), setInputPortsDel(), {.id = SNES_JOYPAD}},
		{"Superscope",  attachParams(), setInputPortsDel(), {.id = SNES_SUPERSCOPE}},
		{"Justifier",   attachParams(), setInputPortsDel(), {.id = SNES_JUSTIFIER}},
		{"Mouse",       attachParams(), setInputPortsDel(), {.id = SNES_MOUSE_SWAPPED}},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", attachParams(),
		MenuId{system().snesInputPort},
		inputPortsItem
	};

	TextMenuItem::SelectDelegate setInputPortsDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionInputPort = item.id;
			system().snesInputPort = item.id;
			system().setupSNESInput(app().defaultVController());
		};
	}

	TextMenuItem videoSystemItem[4]
	{
		{"Auto",             attachParams(), [this](Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC",             attachParams(), [this](Input::Event e){ setVideoSystem(1, e); }},
		{"PAL",              attachParams(), [this](Input::Event e){ setVideoSystem(2, e); }},
		{"NTSC + PAL Spoof", attachParams(), [this](Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"System", attachParams(),
		system().optionVideoSystem.value(),
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		system().sessionOptionSet();
		system().optionVideoSystem = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	TextHeadingMenuItem videoHeading{"Video", attachParams()};

	BoolMenuItem allowExtendedLines
	{
		"Allow Extended 239/478 Lines", attachParams(),
		(bool)system().optionAllowExtendedVideoLines,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionAllowExtendedVideoLines = item.flipBoolValue(*this);
		}
	};

	TextMenuItem deinterlaceModeItems[2]
	{
		{"Bob",   attachParams(), {.id = DeinterlaceMode::Bob}},
		{"Weave", attachParams(), {.id = DeinterlaceMode::Weave}},
	};

	MultiChoiceMenuItem deinterlaceMode
	{
		"Deinterlace Mode", attachParams(),
		MenuId{system().deinterlaceMode},
		deinterlaceModeItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				system().sessionOptionSet();
				system().deinterlaceMode = DeinterlaceMode(item.id.val);
			}
		}
	};

	#ifndef SNES9X_VERSION_1_4
	TextHeadingMenuItem emulationHacks{"Emulation Hacks", attachParams()};

	BoolMenuItem blockInvalidVRAMAccess
	{
		"Allow Invalid VRAM Access", attachParams(),
		(bool)!system().optionBlockInvalidVRAMAccess,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionBlockInvalidVRAMAccess = !item.flipBoolValue(*this);
			PPU.BlockInvalidVRAMAccess = system().optionBlockInvalidVRAMAccess;
		}
	};

	BoolMenuItem separateEchoBuffer
	{
		"Separate Echo Buffer From Ram", attachParams(),
		(bool)system().optionSeparateEchoBuffer,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionSeparateEchoBuffer = item.flipBoolValue(*this);
			SNES::dsp.spc_dsp.separateEchoBuffer = system().optionSeparateEchoBuffer;
		}
	};

	void setSuperFXClock(unsigned val)
	{
		system().sessionOptionSet();
		system().optionSuperFXClockMultiplier = val;
		setSuperFXSpeedMultiplier(system().optionSuperFXClockMultiplier);
	}

	TextMenuItem superFXClockItem[2]
	{
		{"100%", attachParams(), [this]() { setSuperFXClock(100); }},
		{"Custom Value", attachParams(),
			[this](Input::Event e)
			{
				pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 5 to 250", "",
					[this](CollectTextInputView&, auto val)
					{
						if(system().optionSuperFXClockMultiplier.isValid(val))
						{
							setSuperFXClock(val);
							superFXClock.setSelected(lastIndex(superFXClockItem), *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app().postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	};

	MultiChoiceMenuItem superFXClock
	{
		"SuperFX Clock Multiplier", attachParams(),
		[this]()
		{
			if(system().optionSuperFXClockMultiplier == 100)
				return 0;
			else
				return 1;
		}(),
		superFXClockItem,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(std::format("{}%", system().optionSuperFXClockMultiplier.value()));
				return true;
			}
		},
	};
	#endif

	std::array<MenuItem*, IS_SNES9X_VERSION_1_4 ? 6 : 10> menuItem
	{
		&inputPorts,
		&multitap,
		&videoHeading,
		&videoSystem,
		&allowExtendedLines,
		&deinterlaceMode,
		#ifndef SNES9X_VERSION_1_4
		&emulationHacks,
		&blockInvalidVRAMAccess,
		&separateEchoBuffer,
		&superFXClock,
		#endif
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class WRAMViewerView : public TableView, public MainAppHelper
{
	using MainAppHelper::system;

	static constexpr size_t WRAM_SIZE = 0x20000; // 128KB WRAM
	static constexpr size_t ITEMS_PER_PAGE = 64;
	static constexpr size_t WRAM_INIT_ADDRESS = 0x7e0000;

	size_t currentAddress = WRAM_INIT_ADDRESS;
	bool showHex = true;

	TextHeadingMenuItem addressHeading{"Address Range", attachParams()};

	DualTextMenuItem addressRange
	{
		"Current Range",
		std::format("${:06X} - ${:06X}", currentAddress, currentAddress + (ITEMS_PER_PAGE * 8) - 1),
		attachParams(),
		[this](Input::Event e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
				"Enter Address (hex)", std::format("{:X}", currentAddress),
				[this](CollectTextInputView&, auto str)
				{
					unsigned addr = strtoul(str, nullptr, 16); // ← parseHex()で16進数をパース
					if(addr > WRAM_INIT_ADDRESS + WRAM_SIZE  - ITEMS_PER_PAGE * 8 || addr < 0x7e0000) // ← 範囲チェック追加
					{
						app().postMessage(true, "Address out of range");
						return false;
					}
					currentAddress = addr;
					updateDisplay();
					return true;
				});
		}
	};

	BoolMenuItem displayMode
	{
		"Display Mode", attachParams(),
		showHex,
		[this](BoolMenuItem &item)
		{
			showHex = item.flipBoolValue(*this);
			updateDisplay();
		}
	};

	TextHeadingMenuItem dataHeading{"WRAM Data", attachParams()};

	std::array<DualTextMenuItem, ITEMS_PER_PAGE> wramItems;

	void updateDisplay()
	{
		if(!system().hasContent()) return;

		addressRange.set2ndName(std::format("${:06X} - ${:06X}",
			currentAddress, currentAddress + (ITEMS_PER_PAGE * 8) - 1));

		for(size_t i = 0; i < ITEMS_PER_PAGE && (currentAddress + i * 8) < WRAM_INIT_ADDRESS + WRAM_SIZE; i++)
		{
			size_t addr = currentAddress + i * 8;
			std::string addrStr = ((addr & 0xF) == 0) ? std::format("${:06X}:", addr) : "";
			std::string dataStr;

			for(int j = 0; j < 8 && (addr + j) < WRAM_INIT_ADDRESS + WRAM_SIZE; j++)
			{
				size_t physicalAddr = (addr + j) - WRAM_INIT_ADDRESS;
				uint8 value = Memory.RAM[physicalAddr];
				if(showHex)
				{
					dataStr += std::format("{:02X} ", value);
				}
				else
				{
					dataStr += std::format("{:3d} ", value);
				}
			}

			wramItems[i].compile(addrStr);
			wramItems[i].set2ndName(dataStr);
			wramItems[i].place();
		}

		displayMode.set2ndName(showHex ? "Hex" : "Dec");
		displayMode.place();
	}


	std::unique_ptr<TableView> makeByteSelectionView(size_t baseAddr)  
	{  
		// 8バイト分のTextMenuItemを動的に作成  
		auto byteItems = std::make_shared<std::array<TextMenuItem, 8>>();  
		  
		for(int j = 0; j < 8; j++)  
		{  
			size_t byteAddr = baseAddr + j;  
			if(byteAddr >= WRAM_INIT_ADDRESS + WRAM_SIZE) break;  
			  
			size_t physicalAddr = byteAddr - WRAM_INIT_ADDRESS;  
			uint8 value = Memory.RAM[physicalAddr];  
			  
			(*byteItems)[j] = TextMenuItem  
			{  
				std::format("${:06X}: {:02X}", byteAddr, value),  
				attachParams(),  
				[this, byteAddr](Input::Event e)  
				{  
					size_t physicalAddr = byteAddr - WRAM_INIT_ADDRESS;  
					uint8 currentValue = Memory.RAM[physicalAddr];  
					  
					pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,  
						std::format("Edit Address ${:06X}", byteAddr),   
						std::format("{}", currentValue),  
						[this, physicalAddr](CollectTextInputView&, auto str)  
						{
							unsigned val = parseHex(str); // 16進数をパース 
							if(val > 0xFF)
							{
								app().postMessage(true, "Value must be <= FF");
								return false;
							}
							if(physicalAddr < WRAM_SIZE)  
							{  
								Memory.RAM[physicalAddr] = static_cast<uint8>(val & 0xFF);  
								updateDisplay();
								(*byteItems)[j].compile(std::format("${:06X}: {:02X}", byteAddr, static_cast<uint8>(val & 0xFF))); 
							}  
							return true;  
						});  
				}  
			};  
		}  
  
		// TableViewを作成して返す  
		return std::make_unique<TableView>  
		(  
			std::format("Select Byte (${:06X}-${:06X})", baseAddr, baseAddr + 7),  
			attachParams(), 
			[byteItems](TableView::ItemMessage msg) -> TableView::ItemReply  
			{  
				return msg.visit(overloaded  
				{  
					[&](const TableView::ItemsMessage&) -> TableView::ItemReply { return 8u; },  
					[&](const TableView::GetItemMessage& m) -> TableView::ItemReply   
					{   
						return m.idx < 8 ? &(*byteItems)[m.idx] : nullptr;   
					},  
				});  
			}  
		);  
	}




		

	std::array<MenuItem*, 4 + ITEMS_PER_PAGE> menuItems;

public:
	WRAMViewerView(ViewAttachParams attach):
		TableView{"WRAM Viewer", attach, menuItems}
	{
		// メニューアイテムの初期化
		for(size_t i = 0; i < ITEMS_PER_PAGE; i++)
		{
			wramItems[i] = DualTextMenuItem
			{
				"", "", attachParams(),
				[this, i](Input::Event e)
				{
					size_t addr = currentAddress + i * 8;
					pushAndShow(makeByteSelectionView(addr), e);
				}
			};
		}

		// メニュー配列の設定
		menuItems[0] = &addressHeading;
		menuItems[1] = &addressRange;
		menuItems[2] = &displayMode;
		menuItems[3] = &dataHeading;

		for(size_t i = 0; i < ITEMS_PER_PAGE; i++)
		{
			menuItems[4 + i] = &wramItems[i];
		}
	}

	void onShow() override
	{
		TableView::onShow();
		updateDisplay();
	}
};

class CustomSystemActionsView : public SystemActionsView
{
private:
	TextMenuItem options
	{
		"Console Options", attachParams(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
	};
	TextMenuItem wramViewer
	{
		"WRAM Viewer", attachParams(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<WRAMViewerView>(), e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): SystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		item.emplace_back(&wramViewer);
		loadStandardItems();
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper
{
	using MainAppHelper::system;
	using MainAppHelper::app;

	TextMenuItem cheatsPath
	{
		cheatsMenuName(appContext(), system().cheatsDir), attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Cheats", system().userPath(system().cheatsDir),
				[this](CStringView path)
				{
					logMsg("set cheats path:%s", path.data());
					system().cheatsDir = path;
					cheatsPath.compile(cheatsMenuName(appContext(), path));
				}), e);
		}
	};

	TextMenuItem patchesPath
	{
		patchesMenuName(appContext(), system().patchesDir), attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Patches", system().userPath(system().patchesDir),
				[this](CStringView path)
				{
					logMsg("set patches path:%s", path.data());
					system().patchesDir = path;
					patchesPath.compile(patchesMenuName(appContext(), path));
				}), e);
		}
	};

	static std::string satMenuName(IG::ApplicationContext ctx, std::string_view userPath)
	{
		return std::format("Satellaview Files: {}", userPathToDisplayName(ctx, userPath));
	}

	TextMenuItem satPath
	{
		satMenuName(appContext(), system().satDir), attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Satellaview Files", system().userPath(system().satDir),
				[this](CStringView path)
				{
					logMsg("set satellaview files path:%s", path.data());
					system().satDir = path;
					satPath.compile(satMenuName(appContext(), path));
				}), e);
		}
	};

	TextMenuItem bsxBios
	{
		bsxMenuName(system().bsxBiosPath), attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView<>>("BS-X BIOS",
				app().validSearchPath(FS::dirnameUri(system().bsxBiosPath)),
				[this](CStringView path, FS::file_type)
				{
					system().bsxBiosPath = path;
					logMsg("set BS-X bios:%s", path.data());
					bsxBios.compile(bsxMenuName(path));
					return true;
				}, Snes9xSystem::hasBiosExtension), e);
		}
	};

	std::string bsxMenuName(CStringView path) const
	{
		return std::format("BS-X BIOS: {}", appContext().fileUriDisplayName(path));
	}

	TextMenuItem sufamiBios
	{
		sufamiMenuName(system().sufamiBiosPath), attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<DataFileSelectView<>>("Sufami Turbo BIOS",
				app().validSearchPath(FS::dirnameUri(system().sufamiBiosPath)),
				[this](CStringView path, FS::file_type)
				{
					system().sufamiBiosPath = path;
					logMsg("set Sufami Turbo bios:%s", path.data());
					sufamiBios.compile(sufamiMenuName(path));
					return true;
				}, Snes9xSystem::hasBiosExtension), e);
		}
	};

	std::string sufamiMenuName(CStringView path) const
	{
		return std::format("Sufami Turbo BIOS: {}", appContext().fileUriDisplayName(path));
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&cheatsPath);
		item.emplace_back(&patchesPath);
		item.emplace_back(&satPath);
		item.emplace_back(&bsxBios);
		item.emplace_back(&sufamiBios);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		#ifndef SNES9X_VERSION_1_4
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach, audio);
		#endif
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::WRAM_VIEWER: return std::make_unique<WRAMViewerView>(attach);
		default: return nullptr;
	}
}

}
