/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2019-2021 MATE Developers
 *
 * These are the timezone names from /usr/share/zoneinfo/zone.tab.

     sed -e '/^#/d' /usr/share/zoneinfo/zone.tab | \
         awk '{ print "N_(\""$3"\");" }' | sort > time-zones.h

 * Further information is available on <https://www.iana.org/time-zones/>.
 *
 * We only place them here so gettext picks them up for translation.
 * Don't include in any C files.
 */
N_("Africa/Abidjan");
N_("Africa/Accra");
N_("Africa/Addis_Ababa");
N_("Africa/Algiers");
N_("Africa/Asmara");
N_("Africa/Bamako");
N_("Africa/Bangui");
N_("Africa/Banjul");
N_("Africa/Bissau");
N_("Africa/Blantyre");
N_("Africa/Brazzaville");
N_("Africa/Bujumbura");
N_("Africa/Cairo");
N_("Africa/Casablanca");
N_("Africa/Ceuta");
N_("Africa/Conakry");
N_("Africa/Dakar");
N_("Africa/Dar_es_Salaam");
N_("Africa/Djibouti");
N_("Africa/Douala");
N_("Africa/El_Aaiun");
N_("Africa/Freetown");
N_("Africa/Gaborone");
N_("Africa/Harare");
N_("Africa/Johannesburg");
N_("Africa/Juba");
N_("Africa/Kampala");
N_("Africa/Khartoum");
N_("Africa/Kigali");
N_("Africa/Kinshasa");
N_("Africa/Lagos");
N_("Africa/Libreville");
N_("Africa/Lome");
N_("Africa/Luanda");
N_("Africa/Lubumbashi");
N_("Africa/Lusaka");
N_("Africa/Malabo");
N_("Africa/Maputo");
N_("Africa/Maseru");
N_("Africa/Mbabane");
N_("Africa/Mogadishu");
N_("Africa/Monrovia");
N_("Africa/Nairobi");
N_("Africa/Ndjamena");
N_("Africa/Niamey");
N_("Africa/Nouakchott");
N_("Africa/Ouagadougou");
N_("Africa/Porto-Novo");
N_("Africa/Sao_Tome");
N_("Africa/Tripoli");
N_("Africa/Tunis");
N_("Africa/Windhoek");
N_("America/Adak");
N_("America/Anchorage");
N_("America/Anguilla");
N_("America/Antigua");
N_("America/Araguaina");
N_("America/Argentina/Buenos_Aires");
N_("America/Argentina/Catamarca");
N_("America/Argentina/Cordoba");
N_("America/Argentina/Jujuy");
N_("America/Argentina/La_Rioja");
N_("America/Argentina/Mendoza");
N_("America/Argentina/Rio_Gallegos");
N_("America/Argentina/Salta");
N_("America/Argentina/San_Juan");
N_("America/Argentina/San_Luis");
N_("America/Argentina/Tucuman");
N_("America/Argentina/Ushuaia");
N_("America/Aruba");
N_("America/Asuncion");
N_("America/Atikokan");
N_("America/Bahia");
N_("America/Bahia_Banderas");
N_("America/Barbados");
N_("America/Belem");
N_("America/Belize");
N_("America/Blanc-Sablon");
N_("America/Boa_Vista");
N_("America/Bogota");
N_("America/Boise");
N_("America/Cambridge_Bay");
N_("America/Campo_Grande");
N_("America/Cancun");
N_("America/Caracas");
N_("America/Cayenne");
N_("America/Cayman");
N_("America/Chicago");
N_("America/Chihuahua");
N_("America/Costa_Rica");
N_("America/Creston");
N_("America/Cuiaba");
N_("America/Curacao");
N_("America/Danmarkshavn");
N_("America/Dawson");
N_("America/Dawson_Creek");
N_("America/Denver");
N_("America/Detroit");
N_("America/Dominica");
N_("America/Edmonton");
N_("America/Eirunepe");
N_("America/El_Salvador");
N_("America/Fortaleza");
N_("America/Fort_Nelson");
N_("America/Glace_Bay");
N_("America/Godthab");
N_("America/Goose_Bay");
N_("America/Grand_Turk");
N_("America/Grenada");
N_("America/Guadeloupe");
N_("America/Guatemala");
N_("America/Guayaquil");
N_("America/Guyana");
N_("America/Halifax");
N_("America/Havana");
N_("America/Hermosillo");
N_("America/Indiana/Indianapolis");
N_("America/Indiana/Knox");
N_("America/Indiana/Marengo");
N_("America/Indiana/Petersburg");
N_("America/Indiana/Tell_City");
N_("America/Indiana/Vevay");
N_("America/Indiana/Vincennes");
N_("America/Indiana/Winamac");
N_("America/Inuvik");
N_("America/Iqaluit");
N_("America/Jamaica");
N_("America/Juneau");
N_("America/Kentucky/Louisville");
N_("America/Kentucky/Monticello");
N_("America/Kralendijk");
N_("America/La_Paz");
N_("America/Lima");
N_("America/Los_Angeles");
N_("America/Lower_Princes");
N_("America/Maceio");
N_("America/Managua");
N_("America/Manaus");
N_("America/Marigot");
N_("America/Martinique");
N_("America/Matamoros");
N_("America/Mazatlan");
N_("America/Menominee");
N_("America/Merida");
N_("America/Metlakatla");
N_("America/Mexico_City");
N_("America/Miquelon");
N_("America/Moncton");
N_("America/Monterrey");
N_("America/Montevideo");
N_("America/Montserrat");
N_("America/Nassau");
N_("America/New_York");
N_("America/Nipigon");
N_("America/Nome");
N_("America/Noronha");
N_("America/North_Dakota/Beulah");
N_("America/North_Dakota/Center");
N_("America/North_Dakota/New_Salem");
N_("America/Ojinaga");
N_("America/Panama");
N_("America/Pangnirtung");
N_("America/Paramaribo");
N_("America/Phoenix");
N_("America/Port-au-Prince");
N_("America/Port_of_Spain");
N_("America/Porto_Velho");
N_("America/Puerto_Rico");
N_("America/Punta_Arenas");
N_("America/Rainy_River");
N_("America/Rankin_Inlet");
N_("America/Recife");
N_("America/Regina");
N_("America/Resolute");
N_("America/Rio_Branco");
N_("America/Santarem");
N_("America/Santiago");
N_("America/Santo_Domingo");
N_("America/Sao_Paulo");
N_("America/Scoresbysund");
N_("America/Sitka");
N_("America/St_Barthelemy");
N_("America/St_Johns");
N_("America/St_Kitts");
N_("America/St_Lucia");
N_("America/St_Thomas");
N_("America/St_Vincent");
N_("America/Swift_Current");
N_("America/Tegucigalpa");
N_("America/Thule");
N_("America/Thunder_Bay");
N_("America/Tijuana");
N_("America/Toronto");
N_("America/Tortola");
N_("America/Vancouver");
N_("America/Whitehorse");
N_("America/Winnipeg");
N_("America/Yakutat");
N_("America/Yellowknife");
N_("Antarctica/Casey");
N_("Antarctica/Davis");
N_("Antarctica/DumontDUrville");
N_("Antarctica/Macquarie");
N_("Antarctica/Mawson");
N_("Antarctica/McMurdo");
N_("Antarctica/Palmer");
N_("Antarctica/Rothera");
N_("Antarctica/Syowa");
N_("Antarctica/Troll");
N_("Antarctica/Vostok");
N_("Arctic/Longyearbyen");
N_("Asia/Aden");
N_("Asia/Almaty");
N_("Asia/Amman");
N_("Asia/Anadyr");
N_("Asia/Aqtau");
N_("Asia/Aqtobe");
N_("Asia/Ashgabat");
N_("Asia/Atyrau");
N_("Asia/Baghdad");
N_("Asia/Bahrain");
N_("Asia/Baku");
N_("Asia/Bangkok");
N_("Asia/Barnaul");
N_("Asia/Beirut");
N_("Asia/Bishkek");
N_("Asia/Brunei");
N_("Asia/Chita");
N_("Asia/Choibalsan");
N_("Asia/Colombo");
N_("Asia/Damascus");
N_("Asia/Dhaka");
N_("Asia/Dili");
N_("Asia/Dubai");
N_("Asia/Dushanbe");
N_("Asia/Famagusta");
N_("Asia/Gaza");
N_("Asia/Hebron");
N_("Asia/Ho_Chi_Minh");
N_("Asia/Hong_Kong");
N_("Asia/Hovd");
N_("Asia/Irkutsk");
N_("Asia/Jakarta");
N_("Asia/Jayapura");
N_("Asia/Jerusalem");
N_("Asia/Kabul");
N_("Asia/Kamchatka");
N_("Asia/Karachi");
N_("Asia/Kathmandu");
N_("Asia/Khandyga");
N_("Asia/Kolkata");
N_("Asia/Krasnoyarsk");
N_("Asia/Kuala_Lumpur");
N_("Asia/Kuching");
N_("Asia/Kuwait");
N_("Asia/Macau");
N_("Asia/Magadan");
N_("Asia/Makassar");
N_("Asia/Manila");
N_("Asia/Muscat");
N_("Asia/Nicosia");
N_("Asia/Novokuznetsk");
N_("Asia/Novosibirsk");
N_("Asia/Omsk");
N_("Asia/Oral");
N_("Asia/Phnom_Penh");
N_("Asia/Pontianak");
N_("Asia/Pyongyang");
N_("Asia/Qatar");
N_("Asia/Qostanay");
N_("Asia/Qyzylorda");
N_("Asia/Riyadh");
N_("Asia/Sakhalin");
N_("Asia/Samarkand");
N_("Asia/Seoul");
N_("Asia/Shanghai");
N_("Asia/Singapore");
N_("Asia/Srednekolymsk");
N_("Asia/Taipei");
N_("Asia/Tashkent");
N_("Asia/Tbilisi");
N_("Asia/Tehran");
N_("Asia/Thimphu");
N_("Asia/Tokyo");
N_("Asia/Tomsk");
N_("Asia/Ulaanbaatar");
N_("Asia/Urumqi");
N_("Asia/Ust-Nera");
N_("Asia/Vientiane");
N_("Asia/Vladivostok");
N_("Asia/Yakutsk");
N_("Asia/Yangon");
N_("Asia/Yekaterinburg");
N_("Asia/Yerevan");
N_("Atlantic/Azores");
N_("Atlantic/Bermuda");
N_("Atlantic/Canary");
N_("Atlantic/Cape_Verde");
N_("Atlantic/Faroe");
N_("Atlantic/Madeira");
N_("Atlantic/Reykjavik");
N_("Atlantic/South_Georgia");
N_("Atlantic/Stanley");
N_("Atlantic/St_Helena");
N_("Australia/Adelaide");
N_("Australia/Brisbane");
N_("Australia/Broken_Hill");
N_("Australia/Currie");
N_("Australia/Darwin");
N_("Australia/Eucla");
N_("Australia/Hobart");
N_("Australia/Lindeman");
N_("Australia/Lord_Howe");
N_("Australia/Melbourne");
N_("Australia/Perth");
N_("Australia/Sydney");
N_("Europe/Amsterdam");
N_("Europe/Andorra");
N_("Europe/Astrakhan");
N_("Europe/Athens");
N_("Europe/Belgrade");
N_("Europe/Berlin");
N_("Europe/Bratislava");
N_("Europe/Brussels");
N_("Europe/Bucharest");
N_("Europe/Budapest");
N_("Europe/Busingen");
N_("Europe/Chisinau");
N_("Europe/Copenhagen");
N_("Europe/Dublin");
N_("Europe/Gibraltar");
N_("Europe/Guernsey");
N_("Europe/Helsinki");
N_("Europe/Isle_of_Man");
N_("Europe/Istanbul");
N_("Europe/Jersey");
N_("Europe/Kaliningrad");
N_("Europe/Kyiv");
N_("Europe/Kirov");
N_("Europe/Lisbon");
N_("Europe/Ljubljana");
N_("Europe/London");
N_("Europe/Luxembourg");
N_("Europe/Madrid");
N_("Europe/Malta");
N_("Europe/Mariehamn");
N_("Europe/Minsk");
N_("Europe/Monaco");
N_("Europe/Moscow");
N_("Europe/Oslo");
N_("Europe/Paris");
N_("Europe/Podgorica");
N_("Europe/Prague");
N_("Europe/Riga");
N_("Europe/Rome");
N_("Europe/Samara");
N_("Europe/San_Marino");
N_("Europe/Sarajevo");
N_("Europe/Saratov");
N_("Europe/Simferopol");
N_("Europe/Skopje");
N_("Europe/Sofia");
N_("Europe/Stockholm");
N_("Europe/Tallinn");
N_("Europe/Tirane");
N_("Europe/Ulyanovsk");
N_("Europe/Uzhgorod");
N_("Europe/Vaduz");
N_("Europe/Vatican");
N_("Europe/Vienna");
N_("Europe/Vilnius");
N_("Europe/Volgograd");
N_("Europe/Warsaw");
N_("Europe/Zagreb");
N_("Europe/Zaporozhye");
N_("Europe/Zurich");
N_("Indian/Antananarivo");
N_("Indian/Chagos");
N_("Indian/Christmas");
N_("Indian/Cocos");
N_("Indian/Comoro");
N_("Indian/Kerguelen");
N_("Indian/Mahe");
N_("Indian/Maldives");
N_("Indian/Mauritius");
N_("Indian/Mayotte");
N_("Indian/Reunion");
N_("Pacific/Apia");
N_("Pacific/Auckland");
N_("Pacific/Bougainville");
N_("Pacific/Chatham");
N_("Pacific/Chuuk");
N_("Pacific/Easter");
N_("Pacific/Efate");
N_("Pacific/Enderbury");
N_("Pacific/Fakaofo");
N_("Pacific/Fiji");
N_("Pacific/Funafuti");
N_("Pacific/Galapagos");
N_("Pacific/Gambier");
N_("Pacific/Guadalcanal");
N_("Pacific/Guam");
N_("Pacific/Honolulu");
N_("Pacific/Kiritimati");
N_("Pacific/Kosrae");
N_("Pacific/Kwajalein");
N_("Pacific/Majuro");
N_("Pacific/Marquesas");
N_("Pacific/Midway");
N_("Pacific/Nauru");
N_("Pacific/Niue");
N_("Pacific/Norfolk");
N_("Pacific/Noumea");
N_("Pacific/Pago_Pago");
N_("Pacific/Palau");
N_("Pacific/Pitcairn");
N_("Pacific/Pohnpei");
N_("Pacific/Port_Moresby");
N_("Pacific/Rarotonga");
N_("Pacific/Saipan");
N_("Pacific/Tahiti");
N_("Pacific/Tarawa");
N_("Pacific/Tongatapu");
N_("Pacific/Wake");
N_("Pacific/Wallis");
