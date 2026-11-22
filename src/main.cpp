#include "main.h"
#include <slint.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <system_error>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
struct Note { std::string id, title, body; };

static std::string read_all(const fs::path &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream out;
    out << f.rdbuf();
    return out.str();
}

static bool atomic_save(const fs::path &p, const std::string &data) {
    const fs::path tmp = p.string() + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
        f.flush();
        if (!f) return false;
    }
    std::error_code ec;
    fs::rename(tmp, p, ec);
#ifdef _WIN32
    if (ec) {
        if (!MoveFileExW(tmp.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return false;
        return true;
    }
#else
    if (ec) return false;
#endif
    return true;
}

static fs::path note_dir() {
#ifdef _WIN32
    if (const char *appdata = std::getenv("APPDATA"))
        return fs::path(appdata) / "PigeonNotes" / "notes";
#endif
    return fs::current_path() / "PigeonNotesData";
}

static std::string make_id() {
    static unsigned counter = 0;
    const auto n = std::chrono::system_clock::now().time_since_epoch().count();
    return std::to_string(n) + "-" + std::to_string(++counter);
}

int main() {
    auto ui = AppWindow::create();
    const auto directory = note_dir();
    std::error_code ec;
    fs::create_directories(directory, ec);

    std::vector<Note> notes;
    if (!ec) {
        for (const auto &item : fs::directory_iterator(directory, ec)) {
            if (!item.is_regular_file() || item.path().extension() != ".pnote") continue;
            const std::string raw = read_all(item.path());
            const auto split = raw.find('\n');
            if (split == std::string::npos) continue;
            notes.push_back({item.path().stem().string(), raw.substr(0, split), raw.substr(split + 1)});
        }
    }
    std::sort(notes.begin(), notes.end(), [](const Note &a, const Note &b){return a.id > b.id;});

    std::vector<size_t> visible;
    std::string search;
    std::string active;
    bool updating = false;

    auto save = [&](const Note &n) {
        const bool ok = atomic_save(directory / (n.id + ".pnote"), n.title + "\n" + n.body);
        ui->set_status(ok ? "Saved locally" : "Save failed - check disk permissions");
    };
    auto refresh = [&]() {
        std::vector<slint::SharedString> names;
        visible.clear();
        for (size_t i = 0; i < notes.size(); ++i) {
            std::string hay = notes[i].title + "\n" + notes[i].body;
            std::string needle = search;
            std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char c){return (char)std::tolower(c);});
            std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c){return (char)std::tolower(c);});
            if (hay.find(needle) == std::string::npos) continue;
            visible.push_back(i);
            names.emplace_back(notes[i].title.empty() ? "Untitled" : notes[i].title);
        }
        ui->set_note_titles(std::make_shared<slint::VectorModel<slint::SharedString>>(std::move(names)));
        int selected = -1;
        for (size_t i = 0; i < visible.size(); ++i)
            if (notes[visible[i]].id == active) selected = (int)i;
        ui->set_selected_index(selected);
    };
    auto show_active = [&]() {
        auto it = std::find_if(notes.begin(), notes.end(), [&](const Note &n){return n.id == active;});
        updating = true;
        ui->set_has_note(it != notes.end());
        ui->set_confirm_delete(false);
        ui->set_note_title(it == notes.end() ? "" : slint::SharedString(it->title));
        ui->set_note_body(it == notes.end() ? "" : slint::SharedString(it->body));
        ui->set_status("Saved locally");
        updating = false;
        refresh();
    };

    ui->on_create_note([&]() {
        Note n{make_id(), "Untitled note", ""};
        notes.insert(notes.begin(), n);
        active = n.id;
        search.clear();
        ui->set_search_query("");
        save(n);
        show_active();
    });
    ui->on_select_note([&](int i) {
        if (i < 0 || static_cast<size_t>(i) >= visible.size()) return;
        active = notes[visible[static_cast<size_t>(i)]].id;
        show_active();
    });
    ui->on_change_title([&](slint::SharedString s) {
        if (updating) return;
        auto it = std::find_if(notes.begin(), notes.end(), [&](const Note &n){return n.id == active;});
        if (it == notes.end()) return;
        it->title = std::string(s);
        save(*it);
        refresh();
    });
    ui->on_change_body([&](slint::SharedString s) {
        if (updating) return;
        auto it = std::find_if(notes.begin(), notes.end(), [&](const Note &n){return n.id == active;});
        if (it == notes.end()) return;
        it->body = std::string(s);
        save(*it);
        if (!search.empty()) refresh();
    });
    ui->on_delete_note([&]() {
        if (active.empty()) return;
        std::error_code remove_error;
        fs::remove(directory / (active + ".pnote"), remove_error);
        if (remove_error) { ui->set_status("Delete failed"); return; }
        notes.erase(std::remove_if(notes.begin(), notes.end(), [&](const Note &n){return n.id == active;}), notes.end());
        active.clear();
        show_active();
    });
    ui->on_search([&](slint::SharedString s) {
        search = std::string(s);
        refresh();
    });
    if (!notes.empty()) active = notes.front().id;
    show_active();
    ui->run();
}
