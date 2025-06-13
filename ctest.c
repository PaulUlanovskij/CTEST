#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include ".NECO/headers/cio.h"
#include ".NECO/headers/da.h"
#include ".NECO/headers/fio.h"
#include ".NECO/headers/str.h"
#include ".NECO/headers/types.h"

typedef struct {
  cstr build_dir;
  cstr src_dir;
} Config;

bool parse_config(cstr path, Config *cfg) {
  vstr_o contents = {};
  cstr_o full_path = path_join(path, "/Testfile");
  if (vstr_from_file(&contents, full_path) == false) {
    log_error("Could not parse test config");
    free(full_path);
    return false;
  }

  free(full_path);

  vstr_da lines = vstr_split_by_char(contents, '\n');
  da_foreach(&lines, line) {
    // add triming whitespaces to sv and cstr
    vstr_da parts = vstr_split_by_char(*line, '=');
    if (parts.length != 2) {
      log_warning("Unexpected amount of args");
      continue;
    }
    if (cstr_cmp_vstr("BUILD_DIR", vstr_trim(parts.items[0])) == true) {
      cfg->build_dir = vstr_to_cstr(vstr_trim(parts.items[1]));
    } else if (cstr_cmp_vstr("SRC_DIR", vstr_trim(parts.items[0])) == true) {
      cfg->src_dir = vstr_to_cstr(vstr_trim(parts.items[1]));
    } else {
      log_warning("Unknown config parameter");
    }

    da_free(&parts);
  }
  free(contents.items);
  log_info("Parsed config successfully");
  return true;
}

void collect_tests(Config cfg, cstr c_file_path) {
  vstr_o file_contents = {};

  if (vstr_from_file(&file_contents, c_file_path) == false) {
    log_error("Failed to read source file contents");
    return;
  }

  int_da test_marks = vstr_index_word(file_contents, "CTEST");
  if (test_marks.length == 0) {
    free(file_contents.items);
    return;
  }

  if (vstr_word_index(file_contents, "int main(") != -1) {
    log_error("Tests are not allowed to be inside of files with 'main' "
              "function. These tests will be ignored.");
    free(file_contents.items);
    da_free(&test_marks);
    return;
  }

  da_foreach(&test_marks, mark_offset) {
    vstr name = vstr_capture_block_by_char(file_contents, *mark_offset, '(', ')');
    if (name.length == 0) {
      break;
    }
    name = vstr_trim(vstr_slice(name, 1, name.length - 1));
    vstr body = vstr_capture_block_by_char(file_contents, *mark_offset, '{', '}');
    if (body.length == 0) {
      break;
    }

    dstr ds = {};
    dstr_append_cstr(&ds, "#include \"");
    for (int i = 0; i < cstr_count_char(cfg.build_dir, '/') - 1; i++) {
      dstr_append_cstr(&ds, "../");
    }
    dstr_append_vstr(&ds, cstr_slice(c_file_path, 2, -1));

    dstr_append_cstr(&ds, "\"\n\nint main(int argc, char**argv)");
    dstr_append_vstr(&ds, body);

    cstr_o name_cstr = vstr_to_cstr(name);

    cstr_o test_src_name = cstr_concat(name_cstr, ".c");
    cstr_o test_src_path = path_join(cfg.build_dir, test_src_name);
    free(test_src_name);
    FILE *file = fopen(test_src_path, "wb");
    if (file == NULL) {
      log_error("Was not able to create a source file for test");
      da_free(&ds);
      free(test_src_path);
      free(name_cstr);
      continue;
    }
    fwrite_vstr(dstr_slice(ds, 0, ds.length - 1), file);
    fclose(file);

    Cmd cmd = {};
    cstr_o test_exe_path = path_join(cfg.build_dir, name_cstr);
    cmd_append(&cmd, "cc", "-o", test_exe_path, test_src_path);
    if (pid_get_exitcode(cmd_create_child(&cmd)) == 0) {
      cmd_append(&cmd, test_exe_path);
      int code = pid_get_exitcode(cmd_create_child(&cmd));
      if (code == 0) {
        printf("[TEST_INFO] Test %s passed.\n", name_cstr);
      } else {
        printf("[TEST_INFO] Test %s failed with code %d.\n", name_cstr, code);
      }
    } else {
      log_error("Failed to compile test source file");
    }

    da_free(&ds);
    free(test_src_path);
    free(name_cstr);
  }
  da_free(&test_marks);
  free(file_contents.items);
}
void dir_collect_tests(cstr path, Config cfg) {
  Dir src_dir = {};
  if (dir_open(path, &src_dir) == false) {
    log_error("Was not able to open source directory");
    exit(1);
  }
  da_foreach(&src_dir, item) {
    cstr_o new_path = path_join(path, item->name);
    if (item->type == DT_DIR) {
      dir_collect_tests(new_path, cfg);
    } else if (cstr_ends_with(item->name, ".c") ||
               cstr_ends_with(item->name, ".h")) {
      collect_tests(cfg, new_path);
    }
    free(new_path);
  }
  da_free(&src_dir);
}
int main(int argc, char **argv) {
  Config cfg = {};
  parse_config(".", &cfg);
  printf("build dir: %s\n", cfg.build_dir);
  printf("source dir: %s\n", cfg.src_dir);
  dir_collect_tests(cfg.src_dir, cfg);

  return 69;
}
