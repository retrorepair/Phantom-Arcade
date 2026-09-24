with open("/tmp/mister_build_test2/support/groovy/phantom_frontend.cpp", "r") as f:
    text = f.read()

start_save = text.find("static void save_state()")
end_config = text.find("static void init_catalog()")

clean_chunk = """static void save_state() {
    FILE *f = fopen("/media/fat/config/phantom_state.ini", "w");
    if (!f) return;
    fprintf(f, "[State]\\n");
    fprintf(f, "category=%d\\n", current_cat);
    fprintf(f, "selected=%d\\n", selected_idx);
    if (selected_idx >= 0 && selected_idx < game_count) {
        fprintf(f, "game_id=%s\\n", games[selected_idx].id);
    }
    fclose(f);
}

static void load_state() {
    FILE *f = fopen("/media/fat/config/phantom_state.ini", "r");
    if (!f) return;
    char line[128];
    char target_id[64] = "";
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "category=", 9) == 0) current_cat = atoi(line + 9);
        else if (strncmp(line, "selected=", 9) == 0) selected_idx = atoi(line + 9);
        else if (strncmp(line, "game_id=", 8) == 0) {
            strncpy(target_id, line + 8, sizeof(target_id) - 1);
            for (int k = 0; target_id[k]; k++) {
                if (target_id[k] == 13 || target_id[k] == 10) { target_id[k] = 0; break; }
            }
        }
    }
    fclose(f);
    if (strlen(target_id) > 0) {
        for (int i = 0; i < game_count; i++) {
            if (strcmp(games[i].id, target_id) == 0) {
                selected_idx = i;
                break;
            }
        }
    }
}

static void load_config() {
    FILE *f = fopen("/media/fat/config/phantom.ini", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PC_SERVER_IP=", 13) == 0) {
            char *val = line + 13;
            while (*val == ' ' || *val == '\\t') val++;
            for (int k = 0; val[k]; k++) {
                if (val[k] == 13 || val[k] == 10) { val[k] = 0; break; }
            }
            if (strlen(val) > 0) strncpy(pc_server_ip, val, sizeof(pc_server_ip) - 1);
        } else if (strncmp(line, "UDP_PORT=", 9) == 0) {
            int p = atoi(line + 9);
            if (p > 0) pc_udp_port = p;
        }
    }
    fclose(f);
}

"""

new_text = text[:start_save] + clean_chunk + text[end_config:]
with open("/tmp/mister_build_test2/support/groovy/phantom_frontend.cpp", "w") as f:
    f.write(new_text)

print("Replacement done cleanly!")
