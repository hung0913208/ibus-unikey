#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <libintl.h>

#include <stdio.h>
#include <string.h>
#include <string>
#include <ibus.h>

#include "engine_private.h"
#include "utils.h"
#include "unikey.h"
#include "vnconv.h"

#define _(string) gettext(string)

const gchar*          Unikey_IMNames[]    = {"Telex", "Vni", "STelex", "STelex2"};
const UkInputMethod   Unikey_IM[]         = {UkTelex, UkVni, UkSimpleTelex, UkSimpleTelex2};
const unsigned int    NUM_INPUTMETHOD     = sizeof(Unikey_IM)/sizeof(Unikey_IM[0]);

const gchar*          Unikey_OCNames[]    = {"Unicode",
                                             "TCVN3",
                                             "VNI Win",
                                             "VIQR",
                                             "CString",
                                             "NCR Decimal",
                                             "NCR Hex"};
const unsigned int    Unikey_OC[]         = {CONV_CHARSET_XUTF8,
                                             CONV_CHARSET_TCVN3,
                                             CONV_CHARSET_VNIWIN,
                                             CONV_CHARSET_VIQR,
                                             CONV_CHARSET_UNI_CSTRING,
                                             CONV_CHARSET_UNIREF,
                                             CONV_CHARSET_UNIREF_HEX};
const unsigned int    NUM_OUTPUTCHARSET   = sizeof(Unikey_OC)/sizeof(Unikey_OC[0]);

static unsigned char WordBreakSyms[] =
{
    ',', ';', ':', '.', '\"', '\'', '!', '?', ' ',
    '<', '>', '=', '+', '-', '*', '/', '\\',
    '_', '~', '`', '@', '#', '$', '%', '^', '&', '(', ')', '{', '}', '[', ']',
    '|'
};

static unsigned char WordAutoCommit[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'b', 'c', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n',
    'p', 'q', 'r', 's', 't', 'v', 'x', 'z',
    'B', 'C', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N',
    'P', 'Q', 'R', 'S', 'T', 'V', 'X', 'Z'
};

static IBusEngineClass* parent_class = NULL;
static IBusConfig*      config       = NULL;

GType ibus_unikey_engine_get_type(void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof(IBusUnikeyEngineClass),
        (GBaseInitFunc)NULL,
        (GBaseFinalizeFunc)NULL,
        (GClassInitFunc)ibus_unikey_engine_class_init,
        NULL,
        NULL,
        sizeof(IBusUnikeyEngine),
        0,
        (GInstanceInitFunc)ibus_unikey_engine_init,
    };

    if (type == 0)
    {
        type = g_type_register_static(IBUS_TYPE_ENGINE,
                                      "IBusUnikeyEngine",
                                      &type_info,
                                      (GTypeFlags)0);
    }

    return type;
}

void ibus_unikey_init(IBusBus* bus)
{
    UnikeySetup();
    config = ibus_bus_get_config(bus);
}

void ibus_unikey_exit()
{
    UnikeyCleanup();
}

static void ibus_unikey_engine_class_init(IBusUnikeyEngineClass* klass)
{
#ifdef DEBUG
    ibus_warning("class_init()");
#endif

    GObjectClass* object_class         = G_OBJECT_CLASS(klass);
    IBusObjectClass* ibus_object_class = IBUS_OBJECT_CLASS(klass);
    IBusEngineClass* engine_class      = IBUS_ENGINE_CLASS(klass);

    parent_class = (IBusEngineClass* )g_type_class_peek_parent(klass);

    object_class->constructor = ibus_unikey_engine_constructor;
    ibus_object_class->destroy = (IBusObjectDestroyFunc)ibus_unikey_engine_destroy;

    engine_class->process_key_event = ibus_unikey_engine_process_key_event;
    engine_class->reset             = ibus_unikey_engine_reset;
    engine_class->enable            = ibus_unikey_engine_enable;
    engine_class->disable           = ibus_unikey_engine_disable;
    engine_class->focus_in          = ibus_unikey_engine_focus_in;
    engine_class->focus_out         = ibus_unikey_engine_focus_out;
    engine_class->property_activate = ibus_unikey_engine_property_activate;
}

static void ibus_unikey_engine_init(IBusUnikeyEngine* unikey)
{
#ifdef DEBUG
    ibus_warning("init()");
#endif

    GValue v = {0};
    gchar* str;
    gboolean succ;
    int i;

    unikey->preeditstr = new std::string();
    unikey_create_default_options(&unikey->ukopt);

// read config value
    // read Input Method
    succ = ibus_config_get_value(config, "engine/Unikey", "InputMethod", &v);
    unikey->im = UNIKEY_DEFAULT_INPUTMETHOD;
    if (succ)
    {
        str = (gchar*)g_value_get_string(&v);
        for (i = 0; i < NUM_INPUTMETHOD; i++)
        {
            if (strcasecmp(str, Unikey_IMNames[i]) == 0)
            {
                unikey->im = Unikey_IM[i];
            }
        }
        g_value_unset(&v);
    } // end read Input Method

    // read Output Charset
    succ = ibus_config_get_value(config, "engine/Unikey", "OutputCharset", &v);

    unikey->oc = UNIKEY_DEFAULT_OUTPUTCHARSET;
    if (succ)
    {
        str = (gchar*)g_value_get_string(&v);
        for (i = 0; i < NUM_OUTPUTCHARSET; i++)
        {
            if (strcasecmp(str, Unikey_OCNames[i]) == 0)
            {
                unikey->oc = Unikey_OC[i];
            }
        }
        g_value_unset(&v);
    } // end read Output Charset

    // read Unikey Option
    // freemarking
    succ = ibus_config_get_value(config, "engine/Unikey/Options", "FreeMarking", &v);
    if (succ)
    {
        unikey->ukopt.freeMarking = g_value_get_boolean(&v);
        g_value_unset(&v);
    }

    // modernstyle
    succ = ibus_config_get_value(config, "engine/Unikey/Options", "ModernStyle", &v);
    if (succ)
    {
        unikey->ukopt.modernStyle = g_value_get_boolean(&v);
        g_value_unset(&v);
    }

    // macroEnabled
    succ = ibus_config_get_value(config, "engine/Unikey/Options", "MacroEnabled", &v);
    if (succ)
    {
        unikey->ukopt.macroEnabled = g_value_get_boolean(&v);
        g_value_unset(&v);
    }

    // spellCheckEnabled
    succ = ibus_config_get_value(config, "engine/Unikey/Options", "SpellCheckEnabled", &v);
    if (succ)
    {
        unikey->ukopt.spellCheckEnabled = g_value_get_boolean(&v);
        g_value_unset(&v);
    }

    // autoNonVnRestore
    succ = ibus_config_get_value(config, "engine/Unikey/Options", "AutoNonVnRestore", &v);
    if (succ)
    {
        unikey->ukopt.autoNonVnRestore = g_value_get_boolean(&v);
        g_value_unset(&v);
    }
    // end read Unikey Option
// end read config value

    ibus_unikey_engine_create_property_list(unikey);
}

static GObject* ibus_unikey_engine_constructor(GType type,
                                               guint n_construct_params,
                                               GObjectConstructParam* construct_params)
{
#ifdef DEBUG
    ibus_warning("constructor()");
#endif

    IBusUnikeyEngine* unikey;
    const gchar* engine_name;

    unikey = (IBusUnikeyEngine*)
        G_OBJECT_CLASS(parent_class)->constructor(type,
                                                  n_construct_params,
                                                  construct_params);

    engine_name = ibus_engine_get_name((IBusEngine*)unikey);

    return (GObject*)unikey;
}

static void ibus_unikey_engine_destroy(IBusUnikeyEngine* unikey)
{
#ifdef DEBUG
    ibus_warning("destroy()");
#endif

    delete unikey->preeditstr;
}

static void ibus_unikey_engine_focus_in(IBusEngine* engine)
{
#ifdef DEBUG
    ibus_warning("focus_in()");
#endif

    IBusUnikeyEngine* unikey = (IBusUnikeyEngine*)engine;

    UnikeySetInputMethod(unikey->im);
    UnikeySetOutputCharset(unikey->oc);

    UnikeySetOptions(&unikey->ukopt);
    ibus_engine_register_properties(engine, unikey->prop_list);

    parent_class->focus_in(engine);
}

static void ibus_unikey_engine_focus_out(IBusEngine* engine)
{
#ifdef DEBUG
    ibus_warning("focus_out()");
#endif

    ibus_unikey_engine_reset(engine);

    parent_class->focus_out(engine);
}

static void ibus_unikey_engine_reset(IBusEngine* engine)
{
#ifdef DEBUG
    ibus_warning("reset()");
#endif

    IBusUnikeyEngine *unikey = (IBusUnikeyEngine*)engine;

    UnikeyResetBuf();
    if (unikey->preeditstr->length() > 0)
    {
        ibus_unikey_engine_commit_string(engine, unikey->preeditstr->c_str());
        ibus_engine_hide_preedit_text(engine);
        unikey->preeditstr->clear();
    }

    parent_class->reset(engine);
}

static void ibus_unikey_engine_enable(IBusEngine* engine)
{
#ifdef DEBUG
    ibus_warning("enable()");
#endif

    parent_class->enable(engine);
}

static void ibus_unikey_engine_disable(IBusEngine* engine)
{
#ifdef DEBUG
    ibus_warning("disable()");
#endif

    parent_class->disable(engine);
}

static void ibus_unikey_engine_property_activate(IBusEngine* engine,
                                                 const gchar* prop_name,
                                                 guint prop_state)
{
    IBusUnikeyEngine* unikey;
    IBusProperty* prop;
    IBusText* label;
    GValue v = {0};
    int i;

    unikey = (IBusUnikeyEngine*)engine;

    // input method active
    if (strncmp(prop_name, "InputMethod", strlen("InputMethod")) == 0)
    {
        for (i=0; i<NUM_INPUTMETHOD; i++)
        {
            if (strcmp(prop_name + strlen("InputMethod")+1,
                       Unikey_IMNames[i]) == 0)
            {
                unikey->im = Unikey_IM[i];

                g_value_init(&v, G_TYPE_STRING);
                g_value_set_string(&v, Unikey_IMNames[i]);
                ibus_config_set_value(config, "engine/Unikey", "InputMethod", &v);

                // update label
                for (int j=0; j<unikey->prop_list->properties->len; j++)
                {
                    prop = ibus_prop_list_get(unikey->prop_list, j);
                    if (prop==NULL)
                        return;
                    else if (strcmp(prop->key, "InputMethod")==0)
                    {
                        label = ibus_text_new_from_static_string(Unikey_IMNames[i]);
                        ibus_property_set_label(prop, label);
                        g_object_unref(label);
                        break;
                    }
                } // end update label

                // update property state
                for (int j=0; j<unikey->menu_im->properties->len; j++)
                {
                    prop = ibus_prop_list_get(unikey->menu_im, j);
                    if (prop==NULL)
                        return;
                    else if (strcmp(prop->key, prop_name)==0)
                        prop->state = PROP_STATE_CHECKED;
                    else
                        prop->state = PROP_STATE_UNCHECKED;
                } // end update property state

                break;
            }
        }
    } // end input method active

    // output charset active
    else if (strncmp(prop_name, "OutputCharset", strlen("OutputCharset")) == 0)
    {
        for (i=0; i<NUM_OUTPUTCHARSET; i++)
        {
            if (strcmp(prop_name+strlen("OutputCharset")+1,
                       Unikey_OCNames[i]) == 0)
            {
                unikey->oc = Unikey_OC[i];

                g_value_init(&v, G_TYPE_STRING);
                g_value_set_string(&v, Unikey_OCNames[i]);
                ibus_config_set_value(config, "engine/Unikey", "OutputCharset", &v);

                // update label
                for (int j=0; j<unikey->prop_list->properties->len; j++)
                {
                    prop = ibus_prop_list_get(unikey->prop_list, j);
                    if (prop==NULL)
                        return;
                    else if (strcmp(prop->key, "OutputCharset")==0)
                    {
                        label = ibus_text_new_from_static_string(Unikey_OCNames[i]);
                        ibus_property_set_label(prop, label);
                        g_object_unref(label);
                        break;
                    }
                } // end update label

                // update property state
                for (int j=0; j<unikey->menu_oc->properties->len; j++)
                {
                    prop = ibus_prop_list_get(unikey->menu_oc, j);
                    if (prop==NULL)
                        return;
                    else if (strcmp(prop->key, prop_name)==0)
                        prop->state = PROP_STATE_CHECKED;
                    else
                        prop->state = PROP_STATE_UNCHECKED;
                } // end update property state

                break;
            }
        }
    } // end output charset active

    // spellcheck active
    else if (strncmp(prop_name, "Spellcheck", strlen("Spellcheck")) == 0)
    {
        unikey->ukopt.spellCheckEnabled = !unikey->ukopt.spellCheckEnabled;

        g_value_init(&v, G_TYPE_BOOLEAN);
        g_value_set_boolean(&v, unikey->ukopt.spellCheckEnabled);
        ibus_config_set_value(config, "engine/Unikey/Options", "SpellCheckEnabled", &v);

        // update state of state
        for (int j = 0; j < unikey->menu_opt->properties->len ; j++)
        {
            prop = ibus_prop_list_get(unikey->menu_opt, j);
            if (prop == NULL)
                return;

            else if (strcmp(prop->key, "Spellcheck") == 0)
            {
                prop->state = (unikey->ukopt.spellCheckEnabled == 1)?
                    PROP_STATE_CHECKED:PROP_STATE_UNCHECKED;
                break;
            }
        } // end update state
    } // end spellcheck active

    // AutoNonVnRestore active
    else if (strncmp(prop_name, "AutoNonVnRestore", strlen("AutoNonVnRestore")) == 0)
    {
        unikey->ukopt.autoNonVnRestore = !unikey->ukopt.autoNonVnRestore;

        g_value_init(&v, G_TYPE_BOOLEAN);
        g_value_set_boolean(&v, unikey->ukopt.autoNonVnRestore);
        ibus_config_set_value(config, "engine/Unikey/Options", "AutoNonVnRestore", &v);

        // update state of state
        for (int j = 0; j < unikey->menu_opt->properties->len ; j++)
        {
            prop = ibus_prop_list_get(unikey->menu_opt, j);
            if (prop == NULL)
                return;

            else if (strcmp(prop->key, "AutoNonVnRestore") == 0)
            {
                prop->state = (unikey->ukopt.autoNonVnRestore == 1)?
                    PROP_STATE_CHECKED:PROP_STATE_UNCHECKED;
                break;
            }
        } // end update state
    } // end AutoNonVnRestore active

    // ModernStyle active
    else if (strncmp(prop_name, "ModernStyle", strlen("ModernStyle")) == 0)
    {
        unikey->ukopt.modernStyle = !unikey->ukopt.modernStyle;

        g_value_init(&v, G_TYPE_BOOLEAN);
        g_value_set_boolean(&v, unikey->ukopt.modernStyle);
        ibus_config_set_value(config, "engine/Unikey/Options", "ModernStyle", &v);

        // update state of state
        for (int j = 0; j < unikey->menu_opt->properties->len ; j++)
        {
            prop = ibus_prop_list_get(unikey->menu_opt, j);
            if (prop == NULL)
                return;

            else if (strcmp(prop->key, "ModernStyle") == 0)
            {
                prop->state = (unikey->ukopt.modernStyle == 1)?
                    PROP_STATE_CHECKED:PROP_STATE_UNCHECKED;
                break;
            }
        } // end update state
    } // end ModernStyle active

    // FreeMarking active
    else if (strncmp(prop_name, "FreeMarking", strlen("FreeMarking")) == 0)
    {
        unikey->ukopt.freeMarking = !unikey->ukopt.freeMarking;

        g_value_init(&v, G_TYPE_BOOLEAN);
        g_value_set_boolean(&v, unikey->ukopt.freeMarking);
        ibus_config_set_value(config, "engine/Unikey/Options", "FreeMarking", &v);

        // update state of state
        for (int j = 0; j < unikey->menu_opt->properties->len ; j++)
        {
            prop = ibus_prop_list_get(unikey->menu_opt, j);
            if (prop == NULL)
                return;

            else if (strcmp(prop->key, "FreeMarking") == 0)
            {
                prop->state = (unikey->ukopt.freeMarking == 1)?
                    PROP_STATE_CHECKED:PROP_STATE_UNCHECKED;
                break;
            }
        } // end update state
    } // end FreeMarking active

    // MacroEnabled active
    else if (strncmp(prop_name, "MacroEnabled", strlen("MacroEnabled")) == 0)
    {
        unikey->ukopt.macroEnabled = !unikey->ukopt.macroEnabled;

        g_value_init(&v, G_TYPE_BOOLEAN);
        g_value_set_boolean(&v, unikey->ukopt.macroEnabled);
        ibus_config_set_value(config, "engine/Unikey/Options", "MacroEnabled", &v);

        // update state of state
        for (int j = 0; j < unikey->menu_opt->properties->len ; j++)
        {
            prop = ibus_prop_list_get(unikey->menu_opt, j);
            if (prop == NULL)
                return;

            else if (strcmp(prop->key, "MacroEnabled") == 0)
            {
                prop->state = (unikey->ukopt.macroEnabled == 1)?
                    PROP_STATE_CHECKED:PROP_STATE_UNCHECKED;
                break;
            }
        } // end update state
    } // end MacroEnabled active


    ibus_unikey_engine_focus_out(engine);
    ibus_unikey_engine_focus_in(engine);
}

static void ibus_unikey_engine_create_property_list(IBusUnikeyEngine* unikey)
{
    IBusProperty* prop;
    IBusText* label,* tooltip;
    gchar name[32];
    int i;

// create input method menu
    unikey->menu_im = ibus_prop_list_new();
    // add item
    for (i = 0; i < NUM_INPUTMETHOD; i++)
    {
        label = ibus_text_new_from_string(Unikey_IMNames[i]);
        tooltip = ibus_text_new_from_string(""); // ?
        sprintf(name, "InputMethod-%s", Unikey_IMNames[i]);
        prop = ibus_property_new(name,
                                 PROP_TYPE_RADIO,
                                 label,
                                 "",
                                 tooltip,
                                 TRUE,
                                 TRUE,
                                 Unikey_IM[i]==unikey->im?PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                                 NULL);
        g_object_unref(label);
        g_object_unref(tooltip);
        ibus_prop_list_append(unikey->menu_im, prop);
    }

// create output charset menu
    unikey->menu_oc = ibus_prop_list_new();
    // add item
    for (i = 0; i < NUM_OUTPUTCHARSET; i++)
    {
        label = ibus_text_new_from_string(Unikey_OCNames[i]);
        tooltip = ibus_text_new_from_string(""); // ?
        sprintf(name, "OutputCharset-%s", Unikey_OCNames[i]);
        prop = ibus_property_new(name,
                                 PROP_TYPE_RADIO,
                                 label,
                                 "",
                                 tooltip,
                                 TRUE,
                                 TRUE,
                                 Unikey_OC[i]==unikey->oc?PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                                 NULL);
        g_object_unref(label);
        g_object_unref(tooltip);
        ibus_prop_list_append(unikey->menu_oc, prop);
    }

// create option menu (for configure unikey)
    unikey->menu_opt = ibus_prop_list_new();
    // add option property

    // --create and add spellcheck property
    label = ibus_text_new_from_string(_("Enable spell check"));
    tooltip = ibus_text_new_from_string(_("If enable, you can decrease mistake when typing"));
    prop = ibus_property_new("Spellcheck",
                             PROP_TYPE_TOGGLE,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             (unikey->ukopt.spellCheckEnabled==1)?
                             PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                             NULL);
    g_object_unref(label);
    g_object_unref(tooltip);
    ibus_prop_list_append(unikey->menu_opt, prop);

    // --create and add autononvnrestore property
    label = ibus_text_new_from_string(_("Auto restore keys with invalid words"));
    tooltip = ibus_text_new_from_string(_("When typing a word not in Vietnamese,\nit will auto restore keystroke into original"));
    prop = ibus_property_new("AutoNonVnRestore",
                             PROP_TYPE_TOGGLE,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             (unikey->ukopt.autoNonVnRestore==1)?
                             PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                             NULL);
    g_object_unref(label);
    g_object_unref(tooltip);
    ibus_prop_list_append(unikey->menu_opt, prop);

    // --create and add modernstyle property
    label = ibus_text_new_from_string(_("Use oà, uý (instead of òa, úy)"));
    tooltip = ibus_text_new_from_string("");
    prop = ibus_property_new("ModernStyle",
                             PROP_TYPE_TOGGLE,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             (unikey->ukopt.modernStyle==1)?
                             PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                             NULL);
    g_object_unref(label);
    g_object_unref(tooltip);
    ibus_prop_list_append(unikey->menu_opt, prop);


    // --create and add freemarking property
    label = ibus_text_new_from_string(_("Allow type with more freedom"));
    tooltip = ibus_text_new_from_string("");
    prop = ibus_property_new("FreeMarking",
                             PROP_TYPE_TOGGLE,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             (unikey->ukopt.freeMarking==1)?
                             PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                             NULL);
    g_object_unref(label);
    g_object_unref(tooltip);
    ibus_prop_list_append(unikey->menu_opt, prop);

   // --create and add macroEnabled property
    label = ibus_text_new_from_string(_("Enable Macro"));
    tooltip = ibus_text_new_from_string("");
    prop = ibus_property_new("MacroEnabled",
                             PROP_TYPE_TOGGLE,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             (unikey->ukopt.macroEnabled==1)?
                             PROP_STATE_CHECKED:PROP_STATE_UNCHECKED,
                             NULL);
    g_object_unref(label);
    g_object_unref(tooltip);
    ibus_prop_list_append(unikey->menu_opt, prop);


// create top menu
    unikey->prop_list = ibus_prop_list_new();
    // add item
    // -- add input method menu
    for (i = 0; i < NUM_INPUTMETHOD; i++)
    {
        if (Unikey_IM[i] == unikey->im)
            break;
    }
    label = ibus_text_new_from_string(Unikey_IMNames[i]);
    tooltip = ibus_text_new_from_string(_("Choose input method"));
    prop = ibus_property_new("InputMethod",
                             PROP_TYPE_MENU,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             PROP_STATE_UNCHECKED,
                             unikey->menu_im);
    g_object_unref(label);
    g_object_unref(tooltip);

    ibus_prop_list_append(unikey->prop_list, prop);

    // -- add output charset menu
    for (i = 0; i < NUM_OUTPUTCHARSET; i++)
    {
        if (Unikey_OC[i] == unikey->oc)
            break;
    }
    label = ibus_text_new_from_string(Unikey_OCNames[i]);
    tooltip = ibus_text_new_from_string(_("Choose output charset"));
    prop = ibus_property_new("OutputCharset",
                             PROP_TYPE_MENU,
                             label,
                             "",
                             tooltip,
                             TRUE,
                             TRUE,
                             PROP_STATE_UNCHECKED,
                             unikey->menu_oc);
    g_object_unref(label);
    g_object_unref(tooltip);

    ibus_prop_list_append(unikey->prop_list, prop);

    // -- add option menu
    label = ibus_text_new_from_string(_("Options"));
    tooltip = ibus_text_new_from_string(_("Options for Unikey"));
    prop = ibus_property_new("Options",
                             PROP_TYPE_MENU,
                             label,
                             "gtk-preferences",
                             tooltip,
                             TRUE,
                             TRUE,
                             PROP_STATE_UNCHECKED,
                             unikey->menu_opt);
    g_object_unref(label);
    g_object_unref(tooltip);

    ibus_prop_list_append(unikey->prop_list, prop);
// end top menu
}

static void ibus_unikey_engine_commit_string(IBusEngine *engine, const gchar *string)
{
    IBusText *text;

    text = ibus_text_new_from_static_string(string);
    ibus_engine_commit_text(engine, text);
    g_object_unref(text);
}

static void ibus_unikey_engine_update_preedit_string(IBusEngine *engine, const gchar *string, gboolean visible)
{
    IBusUnikeyEngine *unikey;
    IBusText *text;
    int len;

    unikey = (IBusUnikeyEngine*)engine;

    text = ibus_text_new_from_static_string(string);
    len = ibus_text_get_length(text);

    // underline text
    ibus_text_append_attribute(text, IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, len);

    // red text if (spellcheck enable && word is not in Vietnamese)
    if (unikey->ukopt.spellCheckEnabled == 1 && UnikeyLastWordIsNonVn())
    {
        ibus_text_append_attribute(text, IBUS_ATTR_TYPE_FOREGROUND, 0xff0000, 0, len);
    }

    // update and display text
    ibus_engine_update_preedit_text(engine, text, len, visible);

    g_object_unref(text);
}

static void ibus_unikey_engine_erase_chars(IBusEngine *engine, int num_chars)
{
    IBusUnikeyEngine* unikey;
    int i, k;
    guchar c;

    unikey = (IBusUnikeyEngine*)engine;
    k = num_chars;

    for ( i = unikey->preeditstr->length()-1; i >= 0 && k > 0; i--)
    {
        c = unikey->preeditstr->at(i);

        // count down if byte is begin byte of utf-8 char
        if (c < (guchar)'\x80' || c >= (guchar)'\xC0')
        {
            k--;
        }
    }

    unikey->preeditstr->erase(i+1);
}

static gboolean ibus_unikey_engine_process_key_event(IBusEngine* engine,
                                                     guint keyval,
                                                     guint keycode,
                                                     guint modifiers)
{
    static IBusUnikeyEngine* unikey;
    static gboolean tmp;

    unikey = (IBusUnikeyEngine*)engine;

    tmp = ibus_unikey_engine_process_key_event_preedit(engine, keyval, keycode, modifiers);

    // check last keyevent with shift
    if (keyval >= IBUS_space && keyval <=IBUS_asciitilde)
    {
        unikey->last_key_with_shift = modifiers & IBUS_SHIFT_MASK;
    }
    else
    {
        unikey->last_key_with_shift = false;
    } // end check last keyevent with shift

    return tmp;
}

static gboolean ibus_unikey_engine_process_key_event_preedit(IBusEngine* engine,
                                                             guint keyval,
                                                             guint keycode,
                                                             guint modifiers)
{
    static IBusUnikeyEngine* unikey;
    static gchar s[6];
    static int n;

    unikey = (IBusUnikeyEngine*)engine;

    if (modifiers & IBUS_RELEASE_MASK)
    {
        return false;
    }

    else if (modifiers & IBUS_CONTROL_MASK
             || modifiers & IBUS_MOD1_MASK // alternate mask
             || keyval == IBUS_Control_L
             || keyval == IBUS_Control_R
             || keyval == IBUS_Tab
             || keyval == IBUS_Return
             || keyval == IBUS_Delete
             || keyval == IBUS_KP_Enter
             || (keyval >= IBUS_Home && keyval <= IBUS_Insert)
             || (keyval >= IBUS_KP_Home && keyval <= IBUS_KP_Delete)
        )
    {
        if (unikey->preeditstr->length() > 0)
        {
            ibus_unikey_engine_commit_string(engine, unikey->preeditstr->c_str());
            ibus_engine_hide_preedit_text(engine);
            unikey->preeditstr->clear();
        }
        ibus_unikey_engine_reset(engine);
        return false;
    }

    else if (keyval >= IBUS_Shift_L && keyval <= IBUS_Hyper_R)
    {
        return false;
    }

    // capture BackSpace
    else if (keyval == IBUS_BackSpace)
    {
        UnikeyBackspacePress();

        if (UnikeyBackspaces == 0 || unikey->preeditstr->empty())
        {
            ibus_unikey_engine_reset(engine);
            return false;
        }
        else
        {
            if (unikey->preeditstr->length() <= UnikeyBackspaces)
            {
                unikey->preeditstr->clear();
                ibus_engine_hide_preedit_text(engine);
                unikey->auto_commit = true;
            }
            else
            {
                ibus_unikey_engine_erase_chars(engine, UnikeyBackspaces);
                ibus_unikey_engine_update_preedit_string(engine, unikey->preeditstr->c_str(), true);
            }

            // change tone position after press backspace
            if (UnikeyBufChars > 0)
            {
                if (unikey->oc == CONV_CHARSET_XUTF8)
                {
                    unikey->preeditstr->append((const gchar*)UnikeyBuf, UnikeyBufChars);
                }
                else
                {
                    static unsigned char buf[1024];
                    int bufSize = sizeof(buf)/sizeof(buf[0]);

                    latinToUtf(buf, UnikeyBuf, UnikeyBufChars, &bufSize);
                    unikey->preeditstr->append((const gchar*)buf, sizeof(buf)/sizeof(buf[0]) - bufSize);
                }

                unikey->auto_commit = false;
                ibus_unikey_engine_update_preedit_string(engine, unikey->preeditstr->c_str(), true);
            }
        }
        return true;
    } // end capture BackSpace

    else if (keyval >=IBUS_KP_Multiply && keyval <=IBUS_KP_9)
    {
        ibus_unikey_engine_commit_string(engine, unikey->preeditstr->c_str());
        ibus_engine_hide_preedit_text(engine);
        unikey->preeditstr->clear();
        ibus_unikey_engine_reset(engine);
        return false;
    }

    // capture ascii printable char
    else if (keyval >= IBUS_space && keyval <=IBUS_asciitilde)
    {
        int i;

        // process keyval

        // auto commit word that never need to change later in preedit string (like consonant - phu am)
        // if macro enabled, then not auto commit. Because macro may change any word
        if (unikey->ukopt.macroEnabled == 0 && (UnikeyAtWordBeginning() || unikey->auto_commit))
        {
            for (i =0; i < sizeof(WordAutoCommit); i++)
            {
                if (keyval == WordAutoCommit[i])
                {
                    UnikeyPutChar(keyval);
                    unikey->auto_commit = true;
                    return false;
                }
            }
        } // end auto commit

        // :TODO: w at begin word

        unikey->auto_commit = false;

        // shift + space event
        if (unikey->last_key_with_shift == false
            && modifiers & IBUS_SHIFT_MASK
            && keyval == IBUS_space
            && !UnikeyAtWordBeginning())
        {
            UnikeyRestoreKeyStrokes();
        } // end shift + space event

        else
        {
            UnikeyFilter(keyval);
        }
        // end process keyval

        // process result of ukengine
        if (UnikeyBackspaces > 0)
        {
            if (unikey->preeditstr->length() <= UnikeyBackspaces)
            {
                unikey->preeditstr->clear();
            }
            else
            {
                ibus_unikey_engine_erase_chars(engine, UnikeyBackspaces);
            }
        }

        if (UnikeyBufChars > 0)
        {
            if (unikey->oc == CONV_CHARSET_XUTF8)
            {
                unikey->preeditstr->append((const gchar*)UnikeyBuf, UnikeyBufChars);
            }
            else
            {
                static unsigned char buf[1024];
                int bufSize = sizeof(buf)/sizeof(buf[0]);

                latinToUtf(buf, UnikeyBuf, UnikeyBufChars, &bufSize);
                unikey->preeditstr->append((const gchar*)buf, sizeof(buf)/sizeof(buf[0]) - bufSize);
            }
        }
        else // if ukengine not process
        {
            static int n;
            static char s[6];

            n = g_unichar_to_utf8(keyval, s); // convert ucs4 to utf8 char
            unikey->preeditstr->append(s, n);
        }
        // end process result of ukengine

        // commit string: if need
        if (unikey->preeditstr->length() > 0)
        {
            static int i;
            for (i = 0; i < sizeof(WordBreakSyms); i++)
            {
                if (WordBreakSyms[i] == unikey->preeditstr->at(unikey->preeditstr->length()-1)
                    && WordBreakSyms[i] == keyval)
                {
                    ibus_unikey_engine_commit_string(engine, unikey->preeditstr->c_str());
                    ibus_engine_hide_preedit_text(engine);
                    unikey->preeditstr->clear();
                    ibus_unikey_engine_reset(engine);
                    return true;
                }
            }
        }
        // end commit string

        ibus_unikey_engine_update_preedit_string(engine, unikey->preeditstr->c_str(), true);
        return true;
    } //end capture printable char

    // non process key
    ibus_unikey_engine_reset(engine);
    return false;
}