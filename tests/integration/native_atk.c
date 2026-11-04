/* SPDX-License-Identifier: BSD-3-Clause */
#include <atk/atk.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned changes;

static void state_changed(AtkObject *object, const gchar *name, gboolean enabled, gpointer data)
{
    assert(ATK_IS_OBJECT(object) && !strcmp(name, "focused") && enabled && data == &changes);
    ++changes;
}

int main(void)
{
    assert(atk_get_major_version() == 2 && atk_get_minor_version() == 62);
    AtkObject *object = g_object_new(ATK_TYPE_OBJECT, NULL);
    assert(object);
    atk_object_set_role(object, ATK_ROLE_PUSH_BUTTON);
    atk_object_set_name(object, "Abrir ação");
    atk_object_set_description(object, "Ficheiro português");
    assert(atk_object_get_role(object) == ATK_ROLE_PUSH_BUTTON);
    assert(!strcmp(atk_object_get_name(object), "Abrir ação"));
    assert(!strcmp(atk_object_get_description(object), "Ficheiro português"));
    g_signal_connect(object, "state-change", G_CALLBACK(state_changed), &changes);
    atk_object_notify_state_change(object, ATK_STATE_FOCUSED, TRUE);
    assert(changes == 1);
    AtkStateSet *states = atk_state_set_new();
    assert(states && atk_state_set_add_state(states, ATK_STATE_FOCUSED));
    assert(atk_state_set_add_state(states, ATK_STATE_ENABLED));
    assert(atk_state_set_contains_state(states, ATK_STATE_FOCUSED));
    assert(atk_state_set_remove_state(states, ATK_STATE_FOCUSED));
    assert(!atk_state_set_contains_state(states, ATK_STATE_FOCUSED));
    assert(atk_state_set_contains_state(states, ATK_STATE_ENABLED));
    g_object_unref(states);
    g_object_unref(object);
    puts("ATK_OBJECT_SIGNALS_STATES_PASS");
    puts("ATK_NATIVE_PASS");
    return 0;
}
