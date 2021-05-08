#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <wchar.h>
#include <ctype.h>
#include "bt/inc/btree.h"
#define WORD_MAX_LEN 200
#define MEAN_MAX_LEN 40000
#define WORD_WC_MAX_LEN 200
#define MEAN_WC_MAX_LEN 40000
#define LINE_WC_MAX_LEN 40000

/* Chú ý: gchar = char, gint = int */

// Custom structure that holds pointers to widgets and user variables
typedef struct
{
    // Add pointers to widgets below
    GtkWidget *w_comboBoxText;
    GtkWidget *w_meaningView;
    GtkWidget *w_dialogAbout;
    GtkWidget *w_dialogAdd;
    GtkWidget *w_dialogDelete;
    GtkWidget *w_entryAddWord;
    GtkWidget *w_addMeaning;
    GtkTextBuffer *addMeaningBuff;
    GtkWidget *w_entryDeleteWord;
    GtkWidget *w_fileChooserDialog;
    GtkWidget *w_errMsgDialog;
    GtkWidget *w_successMsgDialog;
} app_widgets;

BTA *eng_vie;
GtkWidget *searchEntry;
GtkTextBuffer *meaningViewBuff; // Bộ đệm cho GtkTextView meaningView dùng để hiến thị nghĩa

#include "suggest.h"

static void get_widgets(app_widgets *wdgt, GtkBuilder *builder) // Gán các widget cho các trường trong app_widgets
{
    wdgt->w_comboBoxText = GTK_WIDGET(gtk_builder_get_object(builder, "comboBoxText"));
    searchEntry = GTK_WIDGET(gtk_builder_get_object(builder, "searchEntry"));
    wdgt->w_meaningView = GTK_WIDGET(gtk_builder_get_object(builder, "meaningView"));
    meaningViewBuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "meaningViewBuff"));
    wdgt->w_dialogAbout = GTK_WIDGET(gtk_builder_get_object(builder, "dialogAbout"));
    wdgt->w_dialogAdd = GTK_WIDGET(gtk_builder_get_object(builder, "dialogAdd"));
    wdgt->w_dialogDelete = GTK_WIDGET(gtk_builder_get_object(builder, "dialogDelete"));
    wdgt->w_entryAddWord = GTK_WIDGET(gtk_builder_get_object(builder, "entryAddWord"));
    wdgt->w_addMeaning = GTK_WIDGET(gtk_builder_get_object(builder, "addMeaning"));
    wdgt->addMeaningBuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "addMeaningBuff"));
    wdgt->w_entryDeleteWord = GTK_WIDGET(gtk_builder_get_object(builder, "entryDeleteWord"));
    wdgt->w_fileChooserDialog = GTK_WIDGET(gtk_builder_get_object(builder, "fileChooserDialog"));
    wdgt->w_errMsgDialog = GTK_WIDGET(gtk_builder_get_object(builder, "errMsgDialog"));
    wdgt->w_successMsgDialog = GTK_WIDGET(gtk_builder_get_object(builder, "successMsgDialog"));
}

void strLower(char *dest, const char *s)
{
    int i;
    for (i = 0; i < strlen(s); i++)
    {
        dest[i] = tolower(s[i]);
    }
    dest[strlen(s)] = '\0';
}

void trim(char *s)
{
    char *p = s;
    int l = strlen(p);

    while (isspace(p[l - 1]))
        p[--l] = 0;
    while (*p && isspace(*p))
        ++p, --l;

    memmove(s, p, l + 1);
}

gchar * g_utf8_make_valid_custom (const gchar *str,
                   gssize       len)
{
  GString *string;
  const gchar *remainder, *invalid;
  gsize remaining_bytes, valid_bytes;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = strlen (str);

  string = NULL;
  remainder = str;
  remaining_bytes = len;

  while (remaining_bytes != 0) 
    {
      if (g_utf8_validate (remainder, remaining_bytes, &invalid)) 
	break;
      valid_bytes = invalid - remainder;
    
      if (string == NULL) 
	string = g_string_sized_new (remaining_bytes);

      g_string_append_len (string, remainder, valid_bytes);
      /* append U+FFFD REPLACEMENT CHARACTER */
      //g_string_append (string, "\357\277\275");
      g_string_append (string, " ");
      
      remaining_bytes -= valid_bytes + 1;
      remainder = invalid + 1;
    }
  
  if (string == NULL)
    return g_strndup (str, len);
  
  g_string_append_len (string, remainder, remaining_bytes);
  g_string_append_c (string, '\0');

  g_assert (g_utf8_validate (string->str, -1, NULL));

  return g_string_free (string, FALSE);
}

void openBT() // Mở btree
{
    if ((eng_vie = btopn("dict", 0, 0)) == NULL)
    {
        eng_vie = btcrt("dict", 0, 0);
    }
}

void closeBT() //Đóng Btree
{
    if (eng_vie != NULL)
    {
        btcls(eng_vie);
    }
}

void loadFile(app_widgets *wdgt, char *fileName) // Load file dữ liệu
{
    openBT();

    // Set buffer cho meaningViewBuff
    gtk_text_buffer_set_text(meaningViewBuff, "Loading dictionary...\n", -1);

    int wordCount = 0; // count the number of words in dictionary

    wchar_t *tmp_word;    // widechar word
    wchar_t *tmp_meaning; // widechar meaning
    wchar_t *line;
    wchar_t *linePtr;

    char notify[MEAN_MAX_LEN];

    char *word;
    char *meaning;
    char wordLower[WORD_MAX_LEN];

    tmp_word = malloc(WORD_WC_MAX_LEN * sizeof(wchar_t));
    tmp_meaning = malloc(MEAN_WC_MAX_LEN * sizeof(wchar_t));

    word = malloc(WORD_MAX_LEN * sizeof(char));
    meaning = malloc(MEAN_MAX_LEN * sizeof(char));

    line = malloc(LINE_WC_MAX_LEN * sizeof(wchar_t));

    int i;
    BTint j;
    FILE *f;
    if ((f = fopen(fileName, "r")) == NULL)
    {
        gtk_text_buffer_insert_at_cursor(meaningViewBuff, "File open failed!\n", -1);
        return;
    }
    else
    {
        // Find the first word
        do
        {
            // Read a line in dict file
            linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
        } while (linePtr != NULL && line[0] != L'@');

        while (linePtr != NULL)
        {

            if (feof(f))
            {
                return;
            }

            if (wcslen(line) == 0)
            {
                // Read a line in dict file
                linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
                continue;
            }

            // Split word
            for (i = 1; line[i] != L'/' && line[i] != L'\n' && line[i] != L'\0'; i++)
                ;
            wcsncpy(tmp_word, &line[1], i - 1);
            tmp_word[i - 1] = L'\0';

            // Split meaning
            wcsncpy(tmp_meaning, &line[i], wcslen(line) - i + 1);

            // Continuously read lines containing meaning
            // Read a line in dict file
            linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
            while (linePtr != NULL && line[0] != L'@')
            {
                if (wcslen(tmp_meaning) + wcslen(line) > MEAN_MAX_LEN)
                {
                    sprintf(notify, "Meaning exceeded the maximum length. Word: %S\n", tmp_word);
                    gtk_text_buffer_insert_at_cursor(meaningViewBuff, notify, -1);
                    exit(1);
                }
                wcscat(tmp_meaning, line);
                // Read a line in dict file
                linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
            }

            // Convert from widechar strings to char strings
            wcstombs(word, tmp_word, WORD_MAX_LEN * sizeof(char));
            wcstombs(meaning, tmp_meaning, MEAN_MAX_LEN * sizeof(char));


            strLower(wordLower, word);
            trim(wordLower);

            if (bfndky(eng_vie, wordLower, &j) != 0)
            {
                btins(eng_vie, wordLower, meaning, strlen(meaning) * sizeof(char)); // Insert word to Btree
                wordCount++;                                                                                        // Increase wordCount
            }
        }
    }
    fclose(f);
    wordListForSuggest(searchEntry);             // Cập nhật lại đề xuất cho ô tìm kiếm
    wordListForSuggest(wdgt->w_entryDeleteWord); //Cập nhật lại đề xuất cho ô xóa

    //Hàm sprintf gần giống với các hàm printf, fprintf nhưng nó không in ra stdout, file mà nó "in" vào chuỗi
    sprintf(notify, "Loading done. %d words was loaded.", wordCount);

    //Set buffer cho meaningViewBuff là notify ở trên
    gtk_text_buffer_insert_at_cursor(meaningViewBuff, notify, strlen(notify));

    //free used memory
    free(word);
    free(meaning);
    free(tmp_word);
    free(tmp_meaning);
    free(line);

    // Save the dictionary by re-opening
    sprintf(notify, "\nSaving file...\n");

    closeBT();

    //Append notify to meaningViewBuff
    gtk_text_buffer_insert_at_cursor(meaningViewBuff, notify, strlen(notify));
    gtk_text_buffer_insert_at_cursor(meaningViewBuff, "Loading done. Use lookup entry to lookup words.", -1);
}

void lookUp(app_widgets *wdgt, char *word) // Tìm kiếm từ
{
    openBT();                                                       // Mở btree
    char *meaning = (gchar *)malloc(MEAN_MAX_LEN * sizeof(gchar)); // gchar = char nhé
    int i;                                                         // gint = int nhé
    if (bfndky(eng_vie, word, &i) == 0)
    {
        char *meaningUTF;

        btsel(eng_vie, word, meaning, MEAN_MAX_LEN, &i);

        // Vì sau khi tìm kiếm thành công, thì chuỗi meaning đang không ở định dạng UTF8 nên không thể set nó vào meaningViewBuff được
        //Ở đây tôi chuyển nó về dạng UTF8 với hàm g_convert => ít khi bị lỗi như thế nữa. Đang chưa biết sửa làm sao để nó không còn lỗi nữa.
        meaningUTF = g_utf8_make_valid_custom(meaning, -1);

        //Set meaningUTF cho meaningViewBuff
        gtk_text_buffer_set_text(meaningViewBuff, meaningUTF, -1);
    }
    else
    {
        //Set "Not found" vào meaningViewBuff
        gtk_text_buffer_set_text(meaningViewBuff, "Not found", -1);

        // Hiển thị thông báo lỗi
        messageDialog(wdgt, "Khong ton tai tu nay!!!", 1);
    }

    //free memory
    free(meaning);

    //close btree
    closeBT();
}

void autoComplete(GtkWidget *widget) // Tự động hoàn thành từ sau khi ấn Tab
{
    openBT();
    char word[WORD_MAX_LEN], mean[MEAN_MAX_LEN];
    int rsize;
    char *key = (char *)gtk_entry_get_text(GTK_ENTRY(widget));

    btpos(eng_vie, 1);
    while (btseln(eng_vie, word, mean, MEAN_MAX_LEN, &rsize) == 0) // Lặp qua tất cả các từ từ vị trí đâu của btree

        // So sánh key và word(là key của một nút nào đó trong btree) với strlen(key) kí tự đầu tiên
        // Nếu = 0 thì set word cho buffer của searchEntry
        if (strncmp(key, word, strlen(key)) == 0)
        {
            gtk_entry_buffer_set_text(gtk_entry_get_buffer(searchEntry), word, -1);
            break;
        }
    closeBT();
}

void messageDialog(app_widgets *wdgt, char *message, int type) // Hàm xử lý thông báo
{
    if (type == 0)
    {
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(wdgt->w_successMsgDialog), message);
        gtk_dialog_run(GTK_DIALOG(wdgt->w_successMsgDialog));
        gtk_widget_hide(wdgt->w_successMsgDialog);
    }
    else
    {
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), message);
        gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
        gtk_widget_hide(wdgt->w_errMsgDialog);
    }
}

void on_searchEntry_activate(GtkEntry *searchEntry, app_widgets *wdgt) // Hàm xử lý sự kiến ấn Enter ở searchEntry
{
    char *word = gtk_entry_buffer_get_text(gtk_entry_get_buffer(searchEntry));
    if (strlen(word) <= 0)
    {
        messageDialog(wdgt, "Vui long nhap day du!!!", 1);
    }
    else
    {
        lookUp(wdgt, word); // Gọi hàm tìm từ
    }
}

gboolean onEventPressKey(GtkWidget *widget, GdkEventKey *key, app_widgets *wdgt) //Handle Press Key
{
    if (key)
    {
        if (key->keyval == GDK_KEY_Tab)
        {
            //Khi ấn Tab ở ô tìm kiếm sẽ gọi hàm dưới
            autoComplete(widget);
        }
        else if (key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete)
        {
            // Khi xóa từ sẽ đặt rỗng cho meaningViewBuff
            gtk_text_buffer_set_text(meaningViewBuff, "", -1);
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void addWord(app_widgets *wdgt) // Hàm thêm từ
{
    char word[WORD_MAX_LEN];
    char wordLower[WORD_MAX_LEN];
    strcpy(word, gtk_entry_get_text(GTK_ENTRY(wdgt->w_entryAddWord)));

    //Lấy meaning từ TextView
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(wdgt->w_addMeaning);
    char *mean;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    mean = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    int val;

    strLower(wordLower, word);
    trim(wordLower);
    gtk_entry_buffer_set_text(gtk_entry_get_buffer(wdgt->w_entryAddWord), wordLower, -1);
    if (strlen(wordLower) <= 0 || strlen(mean) <= 0)
    {
        messageDialog(wdgt, "Vui long nhap day du!!!", 1);
    }
    else
    {
        if (bfndky(eng_vie, wordLower, &val) == 0)
        {
            messageDialog(wdgt, "Tu da ton tai!!!", 1);
        }
        else
        {
            btins(eng_vie, wordLower, mean, strlen(mean));
            messageDialog(wdgt, "Da them tu moi!!!", 1);
        }
    }

    //Sau khi them xong dat các ô word và mean ở cửa sổ add new word thành rỗng
    gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryAddWord), "");
    gtk_text_buffer_set_text(wdgt->addMeaningBuff, "", -1);
    free(mean);
}

void delWord(app_widgets *wdgt) // Hàm xóa từ
{
    char *word = (gchar *)gtk_entry_get_text(GTK_ENTRY(wdgt->w_entryDeleteWord));
    int i;

    if (strlen(word) <= 0)
    {
        messageDialog(wdgt, "Vui long nhap day du", 1);
    }
    else
    {
        if (bfndky(eng_vie, word, &i) == 0)
        {
            bdelky(eng_vie, word);
            messageDialog(wdgt, "Da xoa!!!", 1);
        }
        else
        {
            messageDialog(wdgt, "Tu khong duoc co chu viet hao hoac tu khong ton tai!!!", 1);
        }
    }

    //Sau khi xóa thành công đặt ô word ở cửa sổ delete a word thành rỗng
    gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryDeleteWord), "");
}

// called when window is closed
void on_window_main_destroy(GObject *object, gpointer user_data)
{
    closeBT();
    g_print("\tBye! Bye!\n");
    gtk_main_quit();
}

void on_btn_clicked(GtkButton *btn, app_widgets *wdgt) // Hàm xử lý sự kiện bấm các nút
{
    if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCancel") == 0)
    {
        gtk_widget_hide(wdgt->w_fileChooserDialog);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAbout") == 0)
    {
        gtk_widget_hide(wdgt->w_dialogAbout);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAdd") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryAddWord), "");
        gtk_text_buffer_set_text(wdgt->addMeaningBuff, "", -1);
        gtk_widget_hide(wdgt->w_dialogAdd);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogDel") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryDeleteWord), "");
        gtk_widget_hide(wdgt->w_dialogDelete);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnLoad") == 0)
    {
        gchar *fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wdgt->w_fileChooserDialog));
        loadFile(wdgt, fileName);
        gtk_widget_hide(wdgt->w_fileChooserDialog);
        free(fileName);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnAddWord") == 0)
    {
        openBT();
        addWord(wdgt);
        wordListForSuggest(searchEntry);
        wordListForSuggest(wdgt->w_entryDeleteWord);
        closeBT();
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnDelWord") == 0)
    {
        openBT();
        delWord(wdgt);
        wordListForSuggest(searchEntry);
        wordListForSuggest(wdgt->w_entryDeleteWord);
        closeBT();
    }
}

void on_menuItem_activate(GtkMenuItem *menuitm, app_widgets *wdgt) // Hàm xử lý sự kiện khi chọn các mục trong menu
{
    if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "loadFile") == 0)
    {
        gtk_widget_show(wdgt->w_fileChooserDialog);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "about") == 0)
    {
        gtk_widget_show(wdgt->w_dialogAbout);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "add") == 0)
    {
        gtk_widget_show(wdgt->w_dialogAdd);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "delete") == 0)
    {
        gtk_widget_show(wdgt->w_dialogDelete);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "quit") == 0)
    {
        closeBT();
        g_print("\tBye! Bye!\n");
        gtk_main_quit();
    }
}

int main(int argc, char *argv[])
{
    ///////////////////////////////////////////////////////////////////

    btinit();

    ///////////////////////////////////////////////////////////////////
    GtkBuilder *builder;
    GtkWidget *window;
    // Instantiate structure, allocating memory for it
    app_widgets *widgets = g_slice_new(app_widgets);

    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("glade/window_main.glade");

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    // Get pointers to widgets here
    //widgets->w_x  = GTK_WIDGET(gtk_builder_get_object(builder, "x"));
    get_widgets(widgets, builder);

    // Widgets pointer are passed to all widget handler functions as the user_data parameter
    gtk_builder_connect_signals(builder, widgets);

    g_object_unref(builder);

    openBT();
    wordListForSuggest(searchEntry);
    wordListForSuggest(widgets->w_entryDeleteWord);
    closeBT();

    gtk_widget_show_all(window);
    gtk_main();
    // Free up widget structure memory
    g_slice_free(app_widgets, widgets);
    ////////////////////////////////////////////////////////////////////
    return 0;
}
