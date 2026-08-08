// ============================================================================
//  PresetBrowser.h
//  カテゴリ / 検索 / 保存 / 削除 / INIT 対応 プリセットブラウザ
//
//  FACTORY プリセットはバイナリ内蔵 (PresetData.h) のため削除も上書きもできない。
//  ユーザープリセットは
//     <UserAppData>/MULTI-OTO/Presets/<Category>/<Name>.motopreset
//  に APVTS の全パラメータを XML で保存する。
// ============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ColorPalette.h"

/** ブラウザ上の 1 エントリ。内蔵プリセットとユーザーファイルの両方を表す。 */
struct PresetRef
{
    juce::String name;
    juce::String category;
    juce::String description;
    bool         isFactory = false;
    int          factoryIndex = -1;   // isFactory のときのみ有効
    juce::File   file;                // ユーザープリセットのときのみ有効
};

class PresetBrowser : public juce::Component
{
public:
    PresetBrowser()
    {
        txtSearch.setTextToShowWhenEmpty ("Search presets...", MOColors::textDim);
        txtSearch.setColour (juce::TextEditor::backgroundColourId, MOColors::knobTrack);
        txtSearch.setColour (juce::TextEditor::textColourId, MOColors::text);
        txtSearch.setColour (juce::TextEditor::outlineColourId, MOColors::panelLine.withAlpha (0.18f));
        txtSearch.onTextChange = [this] { refreshPresetList(); };
        addAndMakeVisible (txtSearch);

        auto styleBtn = [] (juce::TextButton& b, juce::Colour on) {
            b.setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
            b.setColour (juce::TextButton::buttonOnColourId, on);
            b.setColour (juce::TextButton::textColourOffId,  MOColors::text);
            b.setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        };

        btnSave.setButtonText ("SAVE");
        btnInit.setButtonText ("INIT");
        btnClose.setButtonText ("CLOSE");
        styleBtn (btnSave, MOColors::mint);
        styleBtn (btnInit, MOColors::peach);
        styleBtn (btnClose, MOColors::accent);

        addAndMakeVisible (btnSave);
        addAndMakeVisible (btnInit);
        addAndMakeVisible (btnClose);

        categoryListModel.owner = this;
        presetListModel.owner   = this;
        categoryListModel.isCategoryList = true;

        lstCategories.setModel (&categoryListModel);
        lstPresets.setModel (&presetListModel);
        lstCategories.setRowHeight (22);
        lstPresets.setRowHeight (22);
        lstCategories.setColour (juce::ListBox::backgroundColourId, MOColors::well);
        lstPresets.setColour (juce::ListBox::backgroundColourId, MOColors::well);

        addAndMakeVisible (lstCategories);
        addAndMakeVisible (lstPresets);

        btnClose.onClick = [this] { setVisible (false); };
        btnSave.onClick  = [this] { showSaveDialog(); };
        btnInit.onClick  = [this] { showInitDialog(); };
    }

    // ---- ホスト側 (PluginEditor) から差し込むコールバック ----
    std::function<juce::StringArray()>                    getCategories;
    std::function<juce::Array<PresetRef>(juce::String)>   getPresetsForCategory;
    std::function<void(juce::String, juce::String)>       onSaveRequested;   // (category, name)
    std::function<void(PresetRef)>                        onPresetChosen;
    std::function<bool(juce::File)>                       onPresetDeleteRequested;
    std::function<void()>                                 onInitConfirmed;

    void refreshAll()
    {
        categories.clear();
        categories.add (kAllCategories);
        if (getCategories) categories.addArray (getCategories());

        categoryListModel.items = categories;
        lstCategories.updateContent();

        if (selectedCategoryRow >= categories.size()) selectedCategoryRow = 0;
        lstCategories.selectRow (selectedCategoryRow);

        refreshPresetList();
    }

    void visibilityChanged() override
    {
        if (isVisible()) refreshAll();
    }

    void showMessage (const juce::String& title, const juce::String& body)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, title, body, "OK", this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (MOColors::bg.withAlpha (0.94f));

        auto panel = getLocalBounds().reduced (16).toFloat();
        g.setColour (MOColors::panel);
        g.fillRoundedRectangle (panel, 8.0f);
        g.setColour (MOColors::accent.withAlpha (0.65f));
        g.drawRoundedRectangle (panel, 8.0f, 1.5f);

        auto area = getLocalBounds().reduced (26);
        auto titleRow = area.removeFromTop (26);
        g.setColour (MOColors::accent);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText ("PRESETS", titleRow, juce::Justification::centredLeft);

        area.removeFromTop (4);
        area.removeFromTop (28);   // 検索/ボタン行
        area.removeFromTop (16);

        g.setColour (MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText ("CATEGORY", area.getX(),       area.getY() - 13, 160, 12, juce::Justification::left);
        g.drawText ("PRESET",   area.getX() + 172, area.getY() - 13, 220, 12, juce::Justification::left);

        // 選択中プリセットの説明
        if (hoverDescription.isNotEmpty())
        {
            g.setColour (MOColors::textDim);
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
            g.drawFittedText (hoverDescription, descArea, juce::Justification::topLeft, 3);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (26);

        area.removeFromTop (26);   // タイトル
        area.removeFromTop (4);

        auto topRow = area.removeFromTop (28);
        txtSearch.setBounds (topRow.removeFromLeft (220));
        btnSave.setBounds  (topRow.removeFromRight (68));
        topRow.removeFromRight (8);
        btnInit.setBounds  (topRow.removeFromRight (68));
        topRow.removeFromRight (8);
        btnClose.setBounds (topRow.removeFromRight (68));

        area.removeFromTop (16);

        descArea = area.removeFromBottom (42).withTrimmedTop (8);

        lstCategories.setBounds (area.removeFromLeft (160));
        area.removeFromLeft (12);
        lstPresets.setBounds (area);
    }

private:
    static constexpr const char* kAllCategories = "All";

    void refreshPresetList()
    {
        presetListModel.items.clear();
        presetListModel.factoryFlags.clear();
        currentPresets.clear();
        hoverDescription.clear();

        const juce::String cat = (selectedCategoryRow > 0 && selectedCategoryRow < categories.size())
                                   ? categories[selectedCategoryRow]
                                   : juce::String();

        juce::Array<PresetRef> found;
        if (getPresetsForCategory) found = getPresetsForCategory (cat);

        const juce::String filter = txtSearch.getText().trim();

        for (const auto& r : found)
        {
            if (filter.isEmpty() || r.name.containsIgnoreCase (filter)
                                 || r.category.containsIgnoreCase (filter))
            {
                currentPresets.add (r);
                presetListModel.items.add (r.name);
                presetListModel.factoryFlags.add (r.isFactory);
            }
        }

        lstPresets.updateContent();
        lstPresets.deselectAllRows();
        repaint();
    }

    void categoryRowClicked (int row)
    {
        if (row < 0 || row >= categories.size()) return;
        selectedCategoryRow = row;
        refreshPresetList();
    }

    void presetRowSelected (int row)
    {
        if (row < 0 || row >= currentPresets.size()) { hoverDescription.clear(); repaint(); return; }
        hoverDescription = currentPresets[row].description;
        repaint();
    }

    void presetRowActivated (int row)
    {
        if (row >= 0 && row < currentPresets.size() && onPresetChosen)
            onPresetChosen (currentPresets[row]);
    }

    void presetRowRightClicked (int row)
    {
        if (row < 0 || row >= currentPresets.size()) return;

        const PresetRef target = currentPresets[row];
        lstPresets.selectRow (row);

        juce::PopupMenu menu;
        menu.addSectionHeader (target.name);
        menu.addItem (1, "Load");
        menu.addSeparator();
        menu.addItem (2, "Delete...", !target.isFactory);

        juce::Component::SafePointer<PresetBrowser> safeThis (this);

        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetComponent (&lstPresets)
                                .withMousePosition(),
            [safeThis, target] (int result)
            {
                if (safeThis == nullptr) return;

                if (result == 1)
                {
                    if (safeThis->onPresetChosen) safeThis->onPresetChosen (target);
                }
                else if (result == 2 && !target.isFactory)
                {
                    safeThis->confirmDelete (target.file);
                }
            });
    }

    void confirmDelete (const juce::File& target)
    {
        juce::Component::SafePointer<PresetBrowser> safeThis (this);

        juce::AlertWindow::showOkCancelBox (
            juce::MessageBoxIconType::WarningIcon,
            "Delete Preset",
            "Delete the preset \"" + target.getFileNameWithoutExtension() + "\"?\n\n"
            "This cannot be undone.",
            "Yes", "No", this,
            juce::ModalCallbackFunction::create ([safeThis, target] (int result)
            {
                if (result != 1 || safeThis == nullptr) return;

                bool ok = false;
                if (safeThis->onPresetDeleteRequested)
                    ok = safeThis->onPresetDeleteRequested (target);

                if (!ok)
                    safeThis->showMessage ("Delete Failed",
                        "Could not delete the preset file.\n"
                        "It may be read-only or in use by another application.");

                safeThis->refreshAll();
            }));
    }

    void showSaveDialog()
    {
        auto* win = new juce::AlertWindow ("Save Preset",
                                           "Choose a category and enter a preset name.\n"
                                           "OTT COUNT is not stored - it always follows your current setting.",
                                           juce::MessageBoxIconType::NoIcon, this);

        juce::StringArray cats;
        if (getCategories) cats = getCategories();
        cats.add ("User");
        cats.removeDuplicates (true);

        win->addComboBox ("category", cats, "Category");
        if (auto* cb = win->getComboBoxComponent ("category"))
        {
            cb->setEditableText (true);   // 既存から選ぶ / 新規に打ち込む の両対応
            cb->setSelectedItemIndex (juce::jmax (0, cats.indexOf ("User")), juce::dontSendNotification);
        }

        win->addTextEditor ("name", lastSavedName, "Preset Name");
        win->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
        win->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        juce::Component::SafePointer<PresetBrowser> safeThis (this);

        win->enterModalState (true, juce::ModalCallbackFunction::create (
            [safeThis, win] (int result)
            {
                std::unique_ptr<juce::AlertWindow> owned (win);   // new なので必ず破棄する
                if (result != 1 || safeThis == nullptr) return;

                juce::String cat = "User";
                if (auto* cb = owned->getComboBoxComponent ("category"))
                {
                    const auto t = cb->getText().trim();
                    if (t.isNotEmpty()) cat = t;
                }

                const auto name = owned->getTextEditorContents ("name").trim();
                if (name.isEmpty())
                {
                    safeThis->showMessage ("Save Preset", "Please enter a preset name.");
                    return;
                }

                safeThis->lastSavedName = name;
                if (safeThis->onSaveRequested) safeThis->onSaveRequested (cat, name);
                safeThis->refreshAll();
            }), false);
    }

    void showInitDialog()
    {
        juce::Component::SafePointer<PresetBrowser> safeThis (this);

        juce::AlertWindow::showOkCancelBox (
            juce::MessageBoxIconType::WarningIcon,
            "Reset to Initial State",
            "This resets every parameter to its default value.\n"
            "OTT COUNT is left as it is.\n\n"
            "This cannot be undone. Do you want to continue?",
            "Yes", "No", this,
            juce::ModalCallbackFunction::create ([safeThis] (int result)
            {
                if (result == 1 && safeThis != nullptr && safeThis->onInitConfirmed)
                    safeThis->onInitConfirmed();
            }));
    }

    struct SimpleListModel : public juce::ListBoxModel
    {
        juce::StringArray items;
        juce::Array<bool> factoryFlags;
        PresetBrowser* owner = nullptr;
        bool isCategoryList = false;

        int getNumRows() override { return items.size(); }

        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (row < 0 || row >= items.size()) return;

            if (selected)
            {
                g.setColour (MOColors::accent.withAlpha (0.22f));
                g.fillRect (0, 0, w, h);
            }

            const bool fac = (! isCategoryList) && row < factoryFlags.size() && factoryFlags[row];

            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.setColour (selected ? MOColors::accent : (fac ? MOColors::peach : MOColors::text));
            g.drawText (items[row], 10, 0, w - 46, h, juce::Justification::centredLeft, true);

            if (fac)
            {
                g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
                g.setColour (MOColors::textDim);
                g.drawText ("FACTORY", w - 60, 0, 52, h, juce::Justification::centredRight);
            }
        }

        void selectedRowsChanged (int lastRow) override
        {
            if (owner != nullptr && ! isCategoryList) owner->presetRowSelected (lastRow);
        }

        void listBoxItemClicked (int row, const juce::MouseEvent& e) override
        {
            if (owner == nullptr) return;

            if (isCategoryList) { owner->categoryRowClicked (row); return; }

            if (e.mods.isPopupMenu()) owner->presetRowRightClicked (row);
            else                      owner->presetRowSelected (row);
        }

        void backgroundClicked (const juce::MouseEvent&) override {}

        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
        {
            if (owner == nullptr || isCategoryList) return;
            owner->presetRowActivated (row);
        }

        void returnKeyPressed (int row) override
        {
            if (owner == nullptr || isCategoryList) return;
            owner->presetRowActivated (row);
        }
    };

    juce::TextEditor txtSearch;
    juce::TextButton btnSave, btnInit, btnClose;
    juce::ListBox lstCategories, lstPresets;

    SimpleListModel categoryListModel, presetListModel;

    juce::StringArray categories;
    juce::Array<PresetRef> currentPresets;
    int selectedCategoryRow = 0;
    juce::String lastSavedName;
    juce::String hoverDescription;
    juce::Rectangle<int> descArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
