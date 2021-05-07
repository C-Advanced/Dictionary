#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<gtk/gtk.h>
#include<wchar.h>
#include"bt/inc/btree.h"
#define WORD_MAX_LEN 200
#define MEAN_MAX_LEN 40000
#define WORD_WC_MAX_LEN 200
#define MEAN_WC_MAX_LEN 40000
#define LINE_WC_MAX_LEN 40000

// Custom structure that holds pointers to widgets and user variables
typedef struct
{
    // Add pointers to widgets below
    GtkWidget *w_comboBoxText;
    GtkWidget *w_meaningView;
    GtkTextBuffer *meaningViewBuff;
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

#include"autocomplete.h"

static void get_widgets(app_widgets *wdgt, GtkBuilder *builder)
{
    wdgt->w_comboBoxText = GTK_WIDGET(gtk_builder_get_object(builder, "comboBoxText"));
    searchEntry = GTK_WIDGET(gtk_builder_get_object(builder, "searchEntry"));
    wdgt->w_meaningView = GTK_WIDGET(gtk_builder_get_object(builder, "meaningView"));
    wdgt->meaningViewBuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "meaningViewBuff"));
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

void openBT()
{
    if((eng_vie = btopn("dict", 0, 0)) == NULL)
    {
        eng_vie = btcrt("dict", 0, 0);
    }
}

void closeBT()
{
    if(eng_vie != NULL)
    {
        btcls(eng_vie);
    }
}

void loadFile(app_widgets *wdgt, char *fileName)
{
    openBT();
    gtk_text_buffer_set_text (wdgt->meaningViewBuff, "Loading dictionary...\n", -1);

    int wordCount = 0; // count the number of words in dictionary

    wchar_t * tmp_word; // widechar word
    wchar_t * tmp_meaning; // widechar meaning
    wchar_t * line;
    wchar_t * linePtr;

    char notify[MEAN_MAX_LEN];

    char * word;
    char * meaning;

    tmp_word = malloc(WORD_WC_MAX_LEN * sizeof(wchar_t));
    tmp_meaning = malloc(MEAN_WC_MAX_LEN * sizeof(wchar_t));

    word = malloc(WORD_MAX_LEN * sizeof(char));
    meaning = malloc(MEAN_MAX_LEN * sizeof(char));


    line = malloc(LINE_WC_MAX_LEN * sizeof(wchar_t));

    int i, j;
    FILE *f;
    if((f=fopen(fileName,"r"))==NULL)
    {
        gtk_text_buffer_insert_at_cursor (wdgt->meaningViewBuff, "File open failed!\n", -1);
        return;
    }
    else
    {
            // Find the first word
            do {
                // Read a line in dict file
                linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
            } while(linePtr != NULL && line[0] != L'@');

            while(linePtr != NULL) {

                if (feof(f)) {
                    return;
                }

                if (wcslen(line) == 0) {
                    // Read a line in dict file
                    linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
                    continue;
                }

                // Split word
                for (i = 1; line[i] != L'/' && line[i] != L'\n' && line[i] != L'\0'; i++);
                wcsncpy(tmp_word, &line[1], i-1);
                tmp_word[i-1] = L'\0';


                // Split meaning
                wcsncpy(tmp_meaning, &line[i], wcslen(line) - i + 1);

                // Continuously read lines containing meaning
                // Read a line in dict file
                linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
                while (linePtr != NULL && line[0] != L'@') {
                    if (wcslen(tmp_meaning) + wcslen(line) > MEAN_MAX_LEN) {
                        sprintf(notify, "Meaning exceeded the maximum length. Word: %S\n", tmp_word);
                        gtk_text_buffer_insert_at_cursor (wdgt->meaningViewBuff, notify, -1);
                        exit(1);
                    }
                    wcscat(tmp_meaning, line);
                    // Read a line in dict file
                    linePtr = fgetws(line, LINE_WC_MAX_LEN, f);
                }


                // Convert from widechar strings to char strings
                wcstombs(word, tmp_word, WORD_MAX_LEN*sizeof(char));
                wcstombs(meaning, tmp_meaning, MEAN_MAX_LEN*sizeof(char));


                if(bfndky(eng_vie, word, &j) != 0){
                    btins(eng_vie, word, meaning, strlen(meaning));
                    wordCount++;
                }

            }

    }
    fclose(f);
    wordListForAutoComplete(searchEntry);
    wordListForAutoComplete(wdgt->w_entryDeleteWord);
    sprintf(notify, "Loading done. %d words was loaded.", wordCount);
    gtk_text_buffer_insert_at_cursor (wdgt->meaningViewBuff, notify, strlen(notify));


    //free used memory
    free(word);
    free(meaning);
    free(tmp_word);
    free(tmp_meaning);
    free(line);

    // Save the dictionary by re-opening
    sprintf(notify, "\nSaving file...\n");

    closeBT();

    gtk_text_buffer_insert_at_cursor (wdgt->meaningViewBuff, notify, strlen(notify));

    gtk_text_buffer_insert_at_cursor (wdgt->meaningViewBuff, "Loading done. Use lookup entry to lookup words.", -1);
}

void onEventPressKey(GtkWidget *widget,GdkEventKey *key, app_widgets *wdgt) //Handle Press Key
{
    if(key)
    {

        if(key->keyval == GDK_KEY_Return)
        {
            openBT();
            gchar *word = (gchar *) gtk_entry_get_text(GTK_ENTRY(widget));
            gchar mean[MEAN_MAX_LEN];
            gint i;
            if(strlen(word) <= 0)
            {
                gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Vui long nhap day du!!!");
                gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
                gtk_widget_hide(wdgt->w_errMsgDialog);
            }
            else
            {
                if(bfndky(eng_vie, word, &i) == 0)
                {
                    btsel(eng_vie, word, mean, MEAN_MAX_LEN, &i);
                    gtk_text_buffer_set_text(wdgt->meaningViewBuff, mean, -1);
                }
                else
                {
                    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Khong ton tai tu nay!!!");
                    gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
                    gtk_widget_hide(wdgt->w_errMsgDialog);
                }
            }
            closeBT();
        }
        else if(key->keyval == GDK_KEY_Tab)
        {
            openBT();
            char key[WORD_MAX_LEN], mean[MEAN_MAX_LEN];
            int rsize;
            gchar *word = (gchar *) gtk_entry_get_text(GTK_ENTRY(widget));

            btpos(eng_vie,1);
            while(btseln(eng_vie, key, mean, MEAN_MAX_LEN, &rsize) == 0)
                if(strncmp(word, key, strlen(word)) == 0)
                {
                    gtk_entry_set_text(GTK_ENTRY(widget), key);
                    break;
                }
            closeBT();
        }
        else if(key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete)
        {
            gtk_text_buffer_set_text(wdgt->meaningViewBuff, "", -1);
        }
    }

}


void addWord(app_widgets *wdgt)
{
    gchar word[WORD_MAX_LEN];
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (wdgt->w_addMeaning);
    gchar *mean;

    gtk_text_buffer_get_bounds (buffer, &start, &end);
    mean = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
    strcpy(word, gtk_entry_get_text(GTK_ENTRY(wdgt->w_entryAddWord)));
    gint val;
    if(strlen(word) <= 0 || strlen(mean) <= 0)
    {
        gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Vui long nhap day du!!!");
        gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
        gtk_widget_hide(wdgt->w_errMsgDialog);
    }
    else
    {
        if(bfndky(eng_vie, word, &val) == 0)
        {
            gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Tu da ton tai!!!");
            gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
            gtk_widget_hide(wdgt->w_errMsgDialog);
        }
        else
        {
            btins(eng_vie, word, mean, strlen(mean));
            gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_successMsgDialog), "Da them tu moi!!!");
            gtk_dialog_run(GTK_DIALOG(wdgt->w_successMsgDialog));
            gtk_widget_hide(wdgt->w_successMsgDialog);
        }
    }

    gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryAddWord), "");
    gtk_text_buffer_set_text(wdgt->addMeaningBuff, "", -1);
    free(mean);
}

void delWord(app_widgets *wdgt)
{
    gchar *word = (gchar *) gtk_entry_get_text(GTK_ENTRY(wdgt->w_entryDeleteWord));
    gint i;

    if(strlen(word) <= 0)
    {
        gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Vui long nhap day du!!!");
        gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
        gtk_widget_hide(wdgt->w_errMsgDialog);
    }
    else
    {
        if(bfndky(eng_vie, word, &i) == 0)
        {
            bdelky(eng_vie, word);
            gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_successMsgDialog), "Da xoa!!!");
            gtk_dialog_run(GTK_DIALOG(wdgt->w_successMsgDialog));
            gtk_widget_hide(wdgt->w_successMsgDialog);
        }
        else
        {
            gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(wdgt->w_errMsgDialog), "Tu khong ton tai!!!");
            gtk_dialog_run(GTK_DIALOG(wdgt->w_errMsgDialog));
            gtk_widget_hide(wdgt->w_errMsgDialog);
        }
    }
    gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryDeleteWord), "");
}

// called when window is closed
void on_window_main_destroy(GObject *object, gpointer user_data)
{
    closeBT();
    g_print("\tBye! Bye!\n");
    gtk_main_quit();
}

void on_btn_clicked (GtkButton *btn, app_widgets *wdgt)
{
    if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCancel") == 0)
    {
        gtk_widget_hide(wdgt->w_fileChooserDialog);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAbout") == 0)
    {
        gtk_widget_hide(wdgt->w_dialogAbout);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogAdd") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryAddWord), "");
        gtk_text_buffer_set_text(wdgt->addMeaningBuff, "", -1);
        gtk_widget_hide(wdgt->w_dialogAdd);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnCloseDialogDel") == 0)
    {
        gtk_entry_set_text(GTK_ENTRY(wdgt->w_entryDeleteWord), "");
        gtk_widget_hide(wdgt->w_dialogDelete);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnLoad") == 0)
    {
        gchar *fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wdgt->w_fileChooserDialog));
        loadFile(wdgt, fileName);
        gtk_widget_hide(wdgt->w_fileChooserDialog);

    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnAddWord") == 0)
    {
        openBT();
        addWord(wdgt);
        wordListForAutoComplete(searchEntry);
        wordListForAutoComplete(wdgt->w_entryDeleteWord);
        closeBT();
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(btn)), "btnDelWord") == 0)
    {
        openBT();
        delWord(wdgt);
        wordListForAutoComplete(searchEntry);
        wordListForAutoComplete(wdgt->w_entryDeleteWord);
        closeBT();
    }
}

void on_menuItem_activate (GtkMenuItem *menuitm, app_widgets *wdgt)
{

    if(strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "loadFile") == 0)
    {
        gtk_widget_show(wdgt->w_fileChooserDialog);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "about") == 0)
    {
        gtk_widget_show(wdgt->w_dialogAbout);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "add") == 0)
    {
        gtk_widget_show(wdgt->w_dialogAdd);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "delete") == 0)
    {
        gtk_widget_show(wdgt->w_dialogDelete);
    }
    else if(strcmp(gtk_widget_get_name(GTK_WIDGET(menuitm)), "quit") == 0)
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
    GtkBuilder      *builder;
    GtkWidget       *window;
// Instantiate structure, allocating memory for it
    app_widgets     *widgets = g_slice_new(app_widgets);

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
    wordListForAutoComplete(searchEntry);
    wordListForAutoComplete(widgets->w_entryDeleteWord);
    closeBT();

    gtk_widget_show_all(window);
    gtk_main();
// Free up widget structure memory
    g_slice_free(app_widgets, widgets);
////////////////////////////////////////////////////////////////////
    return 0;
}
