/*
 * Parser for EMV cards.
 *
 * Copyright 2023 Leptoptilos <leptoptilos@icloud.com>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "protocols/emv/emv.h"
#include "helpers/nfc_emv_parser.h"

#define TAG "EMV"

bool emv_get_currency_name(uint16_t cur_code, FuriString* currency_name) {
    if(!cur_code) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);

    bool succsess = nfc_emv_parser_get_currency_name(storage, cur_code, currency_name);

    furi_record_close(RECORD_STORAGE);
    return succsess;
}

bool emv_get_country_name(uint16_t country_code, FuriString* country_name) {
    if(!country_code) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);

    bool succsess = nfc_emv_parser_get_country_name(storage, country_code, country_name);

    furi_record_close(RECORD_STORAGE);
    return succsess;
}

bool emv_get_aid_name(const EmvApplication* apl, FuriString* aid_name) {
    const uint8_t len = apl->aid_len;

    if(!len) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);

    bool succsess = nfc_emv_parser_get_aid_name(storage, apl->aid, len, aid_name);

    furi_record_close(RECORD_STORAGE);
    return succsess;
}

bool emv_parse(const EmvData* data, FuriString* parsed_data) {
    FURI_LOG_I("EMV", "Parsing EMV data");
    furi_assert(data);
    bool parsed = false;

    const EmvApplication app = data->emv_application;

    do {
        if(strlen(app.application_label)) {
            furi_string_cat_printf(parsed_data, "\e#%s\n", app.application_label);
        } else if(strlen(app.application_name)) {
            furi_string_cat_printf(parsed_data, "\e#%s\n", app.application_name);
        } else
            furi_string_cat_printf(parsed_data, "\e#%s\n", "EMV");

        if(app.pan_len) {
            FuriString* pan = furi_string_alloc();
            for(uint8_t i = 0; i < app.pan_len; i += 2) {
                furi_string_cat_printf(pan, "%02X%02X ", app.pan[i], app.pan[i + 1]);
            }

            // Cut padding 'F' from card number
            size_t end = furi_string_search_rchar(pan, 'F');
            if(end) furi_string_left(pan, end);
            furi_string_cat_printf(pan, "\n");
            furi_string_cat(parsed_data, pan);

            furi_string_free(pan);
            parsed = true;
        }

        if(strlen(app.cardholder_name)) {
            furi_string_cat_printf(parsed_data, "Cardholder name: %s\n", app.cardholder_name);
            parsed = true;
        }

        if(app.effective_month) {
            char day[] = "--\0";
            if(app.effective_day)
                snprintf(day, 3, "%02X", app.effective_day);
            if(day[1] == '\0') {
                day[1] = day[0];
                day[0] = '0';
            }

            furi_string_cat_printf(
                parsed_data,
                "Effective: %s.%02X.20%02X\n",
                day,
                app.effective_month,
                app.effective_year);

            parsed = true;
        }

        if(app.exp_month) {
            char day[] = "--\0";
            if(app.exp_day)
                snprintf(day, 3, "%02X", app.exp_day);
            if(day[1] == '\0') {
                day[1] = day[0];
                day[0] = '0';
            }

            furi_string_cat_printf(
                parsed_data, "Expires: %s.%02X.20%02X\n", day, app.exp_month, app.exp_year);

            parsed = true;
        }

        FuriString* str = furi_string_alloc();
        bool storage_readed = emv_get_country_name(app.country_code, str);

        if(storage_readed) {
            furi_string_cat_printf(parsed_data, "Country: %s\n", furi_string_get_cstr(str));
            parsed = true;
        }

        storage_readed = emv_get_currency_name(app.currency_code, str);
        if(storage_readed) {
            furi_string_cat_printf(parsed_data, "Currency: %s\n", furi_string_get_cstr(str));
            parsed = true;
        }

        if(app.pin_try_counter != 0xFF) {
            furi_string_cat_printf(parsed_data, "PIN attempts left: %d\n", app.pin_try_counter);
            parsed = true;
        }

        if((app.application_interchange_profile[1] >> 6) & 0b1) {
            furi_string_cat_printf(parsed_data, "Mobile: yes\n");
            parsed = true;
        }

        if(!parsed) furi_string_cat_printf(parsed_data, "No data was parsed\n");

        parsed = true;
    } while(false);

    return parsed;
}