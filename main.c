#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "bt/inc/btree.h"

#define WORD_MAX_LEN 200
#define MEAN_MAX_LEN 40000
#define LINE_MAX_LEN 40000

// Khai báo các con trỏ cho các widget
GtkComboBoxText *comboBoxText;
GtkTextView *meaningView;
GtkDialog *dialogAbout;
GtkDialog *dialogAdd;
GtkDialog *dialogDelete;
GtkEntry *entryAddWord;
GtkTextView *addMeaning;
GtkTextBuffer *addMeaningBuff;
GtkEntry *entryDeleteWord;
GtkFileChooserDialog *fileChooserDialog;
GtkDialog *errMsgDialog;
GtkDialog *successMsgDialog;
GtkEntry *searchEntry;
GtkTextBuffer *meaningViewBuff; // meaningViewBuff là buffer cho việc set meaning lên meaningView. \
                                                            Tại sao dùng GtkTextBuffer mà không set text trực tiếp trên GtkTextView?\
                                                            Vì GtkTextView không thể set text một cách trực tiếp được mà set text phải thông qua GtkTextBuffer

//Khai báo con trỏ kiểu BTA dùng cho B Tree
BTA *eng_vie;

#include "suggest.h"

// Hàm chuyển chuỗi s về chữ thường hết
void strLower(char *dest, const char *s)
{
    int i;
    for (i = 0; i < strlen(s); i++)
    {
        dest[i] = tolower(s[i]);
    }
    dest[strlen(s)] = '\0';
}

// Chuan hoa chuoi
void chuanHoa(char *s)
{
    //chuan hoa dau chuoi
    while (s[0] == ' ')
        strcpy(&s[0], &s[1]);

    //chuan hoa giua chuoi
    for (int i = 0; i < strlen(s); i++)
    {
        if (s[i] == ' ' && s[i + 1] == ' ')
        {
            strcpy(&s[i], &s[i + 1]);
            i--;
        }
    }

    // Chuan hoa cuoi chuoi
    while (s[strlen(s) - 1] == ' ')
        strcpy(&s[strlen(s) - 1], &s[strlen(s)]);
}

// Hàm mở btree
void openBT()
{
    //open btree
    if ((eng_vie = btopn("AnhViet.dat", 0, 0)) == NULL)
    {
        // create btree file
        eng_vie = btcrt("AnhViet.dat", 0, 0);
    }
}

//Hàm đóng btree
void closeBT()
{
    if (eng_vie != NULL)
    {
        //close btree
        btcls(eng_vie);
    }
}

// Load file dữ liệu
void loadFile(char *fileName)
{
    openBT();

    // Set chuỗi "Loading dictionary...\n" cho meaningViewBuff
    gtk_text_buffer_set_text(meaningViewBuff, "Loading dictionary...\n", -1);

    int wordCount = 0; // đếm số từ insert thành công vào btree file

    char *tmp_word;
    char *tmp_meaning;
    char *line;
    char *linePtr;

    char notify[MEAN_MAX_LEN]; // Chuỗi thông báo

    char *word;
    char *meaning;
    char wordLower[WORD_MAX_LEN];

    tmp_word = malloc(WORD_MAX_LEN * sizeof(char));
    tmp_meaning = malloc(MEAN_MAX_LEN * sizeof(char));

    word = malloc(WORD_MAX_LEN * sizeof(char));
    meaning = malloc(MEAN_MAX_LEN * sizeof(char));

    line = malloc(LINE_MAX_LEN * sizeof(char));

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

        // Read a line in dict file
        linePtr = fgets(line, LINE_MAX_LEN, f);

        while (linePtr != NULL)
        {

            if (feof(f))
            {
                return;
            }

            if (strlen(line) == 0)
            {
                // Read a line in dict file
                linePtr = fgets(line, LINE_MAX_LEN, f);
                continue;
            }

            // Split word
            for (i = 1; line[i] != '/' && line[i] != '\n' && line[i] != '\0'; i++)
                ;
            strncpy(tmp_word, &line[1], i - 1);
            tmp_word[i - 1] = '\0';

            // Split meaning
            strncpy(tmp_meaning, &line[i], strlen(line) - i + 1);

            // Continuously read lines containing meaning
            // Read a line in dict file
            linePtr = fgets(line, LINE_MAX_LEN, f);
            while (linePtr != NULL && line[0] != '@')
            {
                if (strlen(tmp_meaning) + strlen(line) > MEAN_MAX_LEN)
                {
                    sprintf(notify, "Meaning exceeded the maximum length. Word: %S\n", tmp_word);
                    gtk_text_buffer_insert_at_cursor(meaningViewBuff, notify, -1);
                    exit(1);
                }
                strcat(tmp_meaning, line);
                // Read a line in dict file
                linePtr = fgets(line, LINE_MAX_LEN, f);
            }

            strcpy(word, tmp_word);
            strcpy(meaning, tmp_meaning);

            strLower(wordLower, word); // convert word to lower
            chuanHoa(wordLower);       // trim wordLower
            chuanHoa(meaning);

            if (bfndky(eng_vie, wordLower, &j) != 0)
            {
                btins(eng_vie, wordLower, meaning, strlen(meaning) * sizeof(char)); // Insert word to Btree
                wordCount++;                                                        // Increase wordCount
            }
        }
    }

    fclose(f);
    buildListSuggest(searchEntry);     // Cập nhật lại đề xuất cho ô tìm kiếm
    buildListSuggest(entryDeleteWord); //Cập nhật lại đề xuất cho ô xóa

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
    gtk_text_buffer_insert_at_cursor(meaningViewBuff, "Loading done. Use search entry to lookup words.", -1);
}

// Tìm kiếm từ
void lookUp(char *word)
{
    // Mở btree
    openBT();

    char wordLower[WORD_MAX_LEN];
    char meaning[MEAN_MAX_LEN];
    int meaningLength;
    char *meaningUTF;

    strLower(wordLower, word); //Chuyển word về dạng chữ thường

    if (btsel(eng_vie, wordLower, meaning, MEAN_MAX_LEN, &meaningLength) == 0) // Tìm kiếm wordLower trong B Tree
    {
        // Gán dấu hiệu kết thúc chuỗi cho meaning
        meaning[meaningLength] = 0;

        // Validate cho meaning thành chuỗi có định dạng UTF8
        meaningUTF = g_utf8_make_valid(meaning, -1);

        //Hiển thị nghĩa ở meaningView thông qua việc set meaningUTF vào meaningViewBuff
        gtk_text_buffer_set_text(meaningViewBuff, meaningUTF, -1);
    }
    else
    {
        //Hiện thị "Not found" ở meaningView
        gtk_text_buffer_set_text(meaningViewBuff, "Not found", -1);

        // Hiển thị thông báo lỗi
        messageDialog("Khong ton tai tu nay!!!", 1);
    }

    //Đóng btree
    closeBT();
}

// Tự động hoàn thành từ sau khi ấn Tab

void autoComplete(GtkEntry *entry)
{
    openBT();

    char word[WORD_MAX_LEN], mean[MEAN_MAX_LEN];
    int rsize;

    gchar *key = (gchar *)gtk_entry_get_text(GTK_ENTRY(entry)); //Lấy chuỗi đã nhập trong searchEntry

    //Cho con trỏ btree về khóa đầu tiên
    btpos(eng_vie, 1);
    while (btseln(eng_vie, word, mean, MEAN_MAX_LEN, &rsize) == 0) // Lặp qua tất cả các từ từ vị trí đâu của btree
        //So sánh key và word(khóa nào đó trong btree)
        if (strncmp(key, word, strlen(key)) == 0) // So sánh strlen(key) kí tự đầu của word và key
        {
            gtk_entry_set_text(searchEntry, word); // Set word vào searchEntry nếu đã khớp strlen(key) kí tự đầu tiên của word và key
            break;                                 // Thoát khỏi lặp
        }
    closeBT();
}

// Hàm xử lý thông báo
void messageDialog(char *message, int type)
{
    if (type == 0) // type = 0 hiển thị thông báo thành công và type = 1 ( != 0) hiển thị thông báo lỗi.
    {
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(successMsgDialog), message);
        gtk_dialog_run(GTK_DIALOG(successMsgDialog));
        gtk_widget_hide(successMsgDialog);
    }
    else
    {
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(errMsgDialog), message);
        gtk_dialog_run(GTK_DIALOG(errMsgDialog));
        gtk_widget_hide(errMsgDialog);
    }
}

// Hàm xử lý nhấn phím Enter (Tìm từ)
void on_searchEntry_activate(GtkEntry *searchEntry, gpointer user_data)
{
    gchar *word = gtk_entry_buffer_get_text(gtk_entry_get_buffer(searchEntry));

    chuanHoa(word);

    if (strlen(word) <= 0)
    {
        messageDialog("Vui long nhap day du!!!", 1);
    }
    else
    {
        lookUp(word); // Tìm từ nếu strlen(word) > 0
    }
}

// Hàm xử lý sự kiện bấm phím ở searchEntry
gboolean onEventKeyInSearchEntry(GtkWidget *entry, GdkEventKey *key, gpointer data)
{
    //Kiểm tra xem có phải đang nhấn nút hay không
    if (key)
    {
        //Kiểm tra xem nút nhấn có phải là Tab hay không
        if (key->keyval == GDK_KEY_Tab)
        {
            //Khi ấn Tab ở searchEntry sẽ gọi hàm tự động hoàn thành từ (giống như trong Terminal)
            autoComplete(GTK_ENTRY(entry));
        }
        //Kiểm tra xem nút nhấn có phải là nút Backspace hay Delete hay không
        else if (key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete)
        {
            // Khi xóa từ sẽ đặt rỗng cho meaningViewBuff
            gtk_text_buffer_set_text(meaningViewBuff, "", -1);
        }
    }

    //Phải trả về là false thì searchEntry mới có thể hiện chữ được. Còn trả về là TRUE thì không hiện chữ được
    return FALSE;
}

// Hàm thêm từ
void addWord()
{
    char wordLower[WORD_MAX_LEN];
    char *word, *meaning;
    int val;

    ///////////////////////////////////
    //Lấy nghĩa từ mean
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(addMeaningBuff, &start); // Lấy vị trí kí tự đầu trong bộ đệm addMeaningBuff của addMeaning
    gtk_text_buffer_get_end_iter(addMeaningBuff, &end);     // Lấy vị trí kí tự hợp lệ cuối  trong bộ đệm addMeaningBuff của addMeaning
    // Hàm khác lấy cả vị trí đầu và vị trí cuối trong bộ đệm là gtk_text_buffer_get_bounds
    meaning = (gchar *)gtk_text_buffer_get_text(addMeaningBuff, &start, &end, 1); // Gán chuỗi lấy ra từ bộ đệm addMeaningBuff của addMeaning cho char * meaning
    //////////////////////////////////

    word = (gchar *)gtk_entry_get_text(GTK_ENTRY(entryAddWord)); // Lấy từ ở add word

    strLower(wordLower, word); // Convert to lower

    chuanHoa(wordLower);
    chuanHoa(meaning);

    gtk_entry_set_text(GTK_ENTRY(entryAddWord), wordLower); // set wordLower vào entryAddWord

    if (strlen(wordLower) <= 0 || strlen(meaning) <= 0)
    {
        messageDialog("Vui long nhap day du!!!", 1);
    }
    else
    {
        if (bfndky(eng_vie, wordLower, &val) == 0) // Tìm kiếm wordLower trong btree
        {
            messageDialog("Tu da ton tai!!!", 1);
        }
        else
        {
            btins(eng_vie, wordLower, meaning, strlen(meaning) * sizeof(char)); // Thêm từ và nghĩa vào btree nếu từ đó chưa tồn tại trong btree
            messageDialog("Da them tu moi!!!", 1);
        }
    }

    //Sau khi thêm xong đặt word và mean ở cửa sổ add new word thành rỗng
    gtk_entry_set_text(GTK_ENTRY(entryAddWord), "");
    gtk_text_buffer_set_text(addMeaningBuff, "", -1);
}

// Hàm xóa từ
void delWord()
{
    char *word = (gchar *)gtk_entry_get_text(GTK_ENTRY(entryDeleteWord));
    int i;

    chuanHoa(word);

    if (strlen(word) <= 0)
    {
        messageDialog("Vui long nhap day du", 1);
    }
    else
    {
        if (bfndky(eng_vie, word, &i) == 0)
        {
            bdelky(eng_vie, word);
            messageDialog("Da xoa!!!", 1);
        }
        else
        {
            messageDialog("Tu khong ton tai!!!", 1);
        }
    }

    //Sau khi xóa thành công đặt ô word ở cửa sổ delete a word thành rỗng
    gtk_entry_set_text(GTK_ENTRY(entryDeleteWord), "");
}

// called when window is closed
void on_window_main_destroy(GObject *object, gpointer user_data)
{
    g_print("\tBye! Bye!\n");
    gtk_main_quit();
}

// Hàm xử lý sự kiện click các nút
void on_btn_clicked(GtkButton *btn, gpointer data)
{
    //So sánh name widget đang được click với name widget của các widget
    if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCancel") == 0)
    {
        gtk_widget_hide(fileChooserDialog);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAbout") == 0)
    {
        gtk_widget_hide(dialogAbout);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAdd") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(entryAddWord), "");
        gtk_text_buffer_set_text(addMeaningBuff, "", -1);
        gtk_widget_hide(dialogAdd);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogDel") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(entryDeleteWord), "");
        gtk_widget_hide(dialogDelete);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnLoad") == 0)
    {
        gchar *fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooserDialog)); // gchar = char
        loadFile(fileName);
        gtk_widget_hide(fileChooserDialog);
        free(fileName);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnAddWord") == 0)
    {
        openBT();
        addWord();
        buildListSuggest(searchEntry);
        buildListSuggest(entryDeleteWord);
        closeBT();
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnDelWord") == 0)
    {
        openBT();
        delWord();
        buildListSuggest(searchEntry);
        buildListSuggest(entryDeleteWord);
        closeBT();
    }
}

// Hàm xử lý sự kiện khi chọn các mục trong menu
void on_menuItem_activate(GtkMenuItem *menuitm, gpointer data)
{
    if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "loadFile") == 0)
    {
        gtk_widget_show(fileChooserDialog);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "about") == 0)
    {
        gtk_widget_show(dialogAbout);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "add") == 0)
    {
        gtk_widget_show(dialogAdd);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "delete") == 0)
    {
        gtk_widget_show(dialogDelete);
    }
    else if (strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "quit") == 0)
    {
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

    // Khởi tạo GTK
    gtk_init(&argc, &argv);

    // Load giao diện từ file .glade vào biến builder
    builder = gtk_builder_new_from_file("glade/window_main.glade");

    // Gán các widget lấy từ builder cho các con trỏ
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    comboBoxText = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "comboBoxText"));
    searchEntry = GTK_ENTRY(gtk_builder_get_object(builder, "searchEntry"));
    meaningView = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "meaningView"));
    meaningViewBuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "meaningViewBuff"));
    dialogAbout = GTK_DIALOG(gtk_builder_get_object(builder, "dialogAbout"));
    dialogAdd = GTK_DIALOG(gtk_builder_get_object(builder, "dialogAdd"));
    dialogDelete = GTK_DIALOG(gtk_builder_get_object(builder, "dialogDelete"));
    entryAddWord = GTK_ENTRY(gtk_builder_get_object(builder, "entryAddWord"));
    addMeaning = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "addMeaning"));
    addMeaningBuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "addMeaningBuff"));
    entryDeleteWord = GTK_ENTRY(gtk_builder_get_object(builder, "entryDeleteWord"));
    fileChooserDialog = GTK_FILE_CHOOSER_DIALOG(gtk_builder_get_object(builder, "fileChooserDialog"));
    errMsgDialog = GTK_DIALOG(gtk_builder_get_object(builder, "errMsgDialog"));
    successMsgDialog = GTK_DIALOG(gtk_builder_get_object(builder, "successMsgDialog"));

    // Gán xử lý các sự kiện cho builder. Ở đây là NULL
    gtk_builder_connect_signals(builder, NULL);

    g_object_unref(builder);

    //Mở btree
    openBT();

    //Cập nhật gợi ý từ cho searchEntry và entryDeleteWord
    buildListSuggest(searchEntry);
    buildListSuggest(entryDeleteWord);

    //Đóng btree
    closeBT();

    // Hiển thị cửa sổ chương trình
    gtk_widget_show_all(window);

    //Gọi vòng lặp cho đến khi hàm gtk_main_quit được gọi
    gtk_main();
    ////////////////////////////////////////////////////////////////////
    return 0;
}