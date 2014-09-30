
// save: 21.09.2014 18:45


#include <iostream>

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "welcome.h"

std::list<char*> DateiListe;

int timespec2str(char *buf, uint len, struct timespec *ts) {
    int ret;
    struct tm t;

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL) {
        return 1;
    }

    ret = strftime(buf, len, "%F %T", &t);
    if (ret == 0) {
        return 2;
    }
    len = len - ret;

    ret = snprintf(&buf[ret], len, ".%09ld", ts->tv_nsec);
    if (ret >= len) {
        return 3;
    }

    return 0;
}

bool istInDateiListe(char *datei) {

    for (std::list<char*>::iterator it = DateiListe.begin(); it != DateiListe.end(); it++) {

        //        printf("*it %s\n", *it);

        if (0 == strcmp(*it, datei)) {
            return true;
        }

    }

    return false;
}

char firstlines[] = "train_id;retransfer_train_id;paket_id;count_pakets_in_train;recv_data_rate;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;last_recv_paket_bytes;timeout_time_tv_sec;timeout_time_tv_usec;recv_time;send_time;rtt\n\n\n";

char recv_str[] = "recv";
char send_str[] = "send";

void log_zeile(FILE *f, char *recv_send_str, paket_header ph) {

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;
    char timestr1[timestr_size];
    char timestr2[timestr_size];

    timespec2str(timestr1, timestr_size, &ph.recv_time);
    timespec2str(timestr2, timestr_size, &ph.send_time);
    fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s;%ld;%ld;%ld;%ld;%f;\n",
            recv_send_str,
            ph.train_id,
            ph.retransfer_train_id,
            ph.paket_id,
            ph.count_pakets_in_train,
            ph.recv_data_rate,

            ph.last_recv_train_id,
            ph.last_recv_retransfer_train_id,
            ph.last_recv_paket_id,

            ph.last_recv_paket_bytes,

            ph.timeout_time_tv_sec,
            ph.timeout_time_tv_usec,

            timestr1,
            timestr2,

            ph.recv_time.tv_sec,
            ph.recv_time.tv_nsec,
            ph.send_time.tv_sec,
            ph.send_time.tv_nsec,

            ph.rrt

            );

    fflush(f);
}

// cvs Datei erstellen, mit allen paketen

void create_csv_datei(char* recvDatei, char* sendDatei, char* csvDatei) {

    int File_Deskriptor_send;
    int File_Deskriptor_recv;
    int File_Deskriptor;

    // O_WRONLY nur zum Schreiben oeffnen
    // O_RDWR zum Lesen und Schreiben oeffnen
    // O_RDONLY nur zum Lesen oeffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei oeffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht geoeffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zur�ckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben geoeffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    if ((File_Deskriptor_send = open(sendDatei, O_RDONLY)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor_recv = open(recvDatei, O_RDONLY, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    int buf_size = strlen(firstlines);
    char buf[buf_size];
    int readed = read(File_Deskriptor_send, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    readed = read(File_Deskriptor_recv, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor = open(csvDatei, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", csvDatei, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    FILE *f = fdopen(File_Deskriptor, "w");
    char firstlines2[] = "type;train_id;retransfer_train_id;paket_id;count_pakets_in_train;recv_data_rate;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;last_recv_paket_bytes;timeout_time_tv_sec;timeout_time_tv_usec;recv_time;send_time;recv_time_sec;recv_time_nsec;send_time_sec;send_time_nsec;rtt\n";
    fprintf(f, "%s", firstlines2);

    bool ende = false;

    paket_header ph_recv;
    paket_header ph_send;
    paket_header * php_recv = NULL;
    paket_header * php_send = NULL;

    int paket_header_size = sizeof (paket_header);

    int total_recv = 0;
    int total_send = 0;

    int x = 0;

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;
    char timestr1[timestr_size];
    char timestr2[timestr_size];

    while (ende == false) {

        if (php_recv == NULL) {
            php_recv = &ph_recv;

            readed = read(File_Deskriptor_recv, php_recv, paket_header_size);

            total_recv = total_recv + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, recvDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_recv = NULL;
                }
            } else {
                x = php_recv->train_id;
            }
        }

        if (php_send == NULL) {
            php_send = &ph_send;

            readed = read(File_Deskriptor_send, php_send, paket_header_size);

            total_send = total_send + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, sendDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_send = NULL;
                }
            } else {
                x = php_send->train_id;
            }
        }

        if (php_recv != NULL) {
            if (php_recv->train_id == 12
                    && php_recv->retransfer_train_id == 0
                    && php_recv->paket_id == 0) {

                php_recv++;
                php_recv--;

            }
        }

        if (php_recv == NULL && php_send == NULL) {
            ende = true;
            continue;
        }

        if (php_recv == NULL || php_send == NULL) {

            if (php_recv != NULL) {

                log_zeile(f, recv_str, ph_recv);

                php_recv = NULL;

            } else {

                log_zeile(f, send_str, ph_send);

                php_send = NULL;

            }

        } else {
            if (ph_recv.recv_time.tv_sec < ph_send.send_time.tv_sec) {

                log_zeile(f, recv_str, ph_recv);

                php_recv = NULL;

            } else if (ph_send.send_time.tv_sec < ph_recv.recv_time.tv_sec) {

                log_zeile(f, send_str, ph_send);

                php_send = NULL;

            } else {

                if (ph_recv.recv_time.tv_nsec < ph_send.send_time.tv_nsec) {

                    log_zeile(f, recv_str, ph_recv);

                    php_recv = NULL;

                } else {

                    log_zeile(f, send_str, ph_send);

                    php_send = NULL;
                }
            }
        }
    }
}

// cvs Datei erstellen (zusammenfassung), nur mit erstem und letztem Paket vom Train

void create_csv_datei_zus(char* recvDatei, char* sendDatei, char* csvDatei) {

    int File_Deskriptor_send;
    int File_Deskriptor_recv;
    int File_Deskriptor;

    // O_WRONLY nur zum Schreiben �ffnen
    // O_RDWR zum Lesen und Schreiben �ffnen
    // O_RDONLY nur zum Lesen �ffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei �ffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht ge�ffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zur�ckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben ge�ffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    if ((File_Deskriptor_send = open(sendDatei, O_RDONLY)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor_recv = open(recvDatei, O_RDONLY, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    int buf_size = strlen(firstlines);
    char buf[buf_size];
    int readed = read(File_Deskriptor_send, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    readed = read(File_Deskriptor_recv, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor = open(csvDatei, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", csvDatei, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    FILE *f = fdopen(File_Deskriptor, "w");
    char firstlines2[] = "type;train_id;retransfer_train_id;paket_id;count_pakets_in_train;recv_data_rate;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;last_paket_recv_bytes;timeout_time_tv_sec;timeout_time_tv_usec;recv_time;send_time;recv_time_sec;recv_time_usec;send_time_sec;send_time_usec\n";
    fprintf(f, "%s", firstlines2);

    bool ende = false;

    paket_header ph_recv;
    paket_header ph_send;
    paket_header * php_recv = NULL;
    paket_header * php_send = NULL;

    int paket_header_size = sizeof (paket_header);

    int total_recv = 0;
    int total_send = 0;

    int x = 0;

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;

    struct paket_header last_ph;
    last_ph.train_id = -1;
    char *last_recv_send;
    char last_recv[] = "recv";
    char last_send[] = "send";

    while (ende == false) {

        if (php_recv == NULL) {
            php_recv = &ph_recv;

            readed = read(File_Deskriptor_recv, php_recv, paket_header_size);

            total_recv = total_recv + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, recvDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_recv = NULL;
                }
            } else {
                x = php_recv->train_id;
            }
        }

        if (php_send == NULL) {
            php_send = &ph_send;

            readed = read(File_Deskriptor_send, php_send, paket_header_size);

            total_send = total_send + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, sendDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_send = NULL;
                }
            } else {
                x = php_send->train_id;
            }
        }

        if (php_recv == NULL && php_send == NULL) {
            ende = true;
            continue;
        }

        if (php_recv == NULL || php_send == NULL) {

            if (php_recv != NULL) {

                {
                    // recv speichern
                    if (last_ph.train_id == -1) {

                        log_zeile(f, recv_str, ph_recv);

                    } else {

                        if (last_ph.train_id == ph_recv.train_id
                                && last_ph.retransfer_train_id == ph_recv.retransfer_train_id
                                && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            log_zeile(f, last_recv_send, last_ph);

                            log_zeile(f, recv_str, ph_recv);

                        }
                    }

                    php_recv = NULL;
                    last_ph = ph_recv;
                    last_recv_send = last_recv;
                    // recv speichern
                }

            } else {

                {
                    // send speichern
                    if (last_ph.train_id == -1) {

                        log_zeile(f, send_str, ph_send);

                    } else {

                        if (last_ph.train_id == ph_send.train_id
                                && last_ph.retransfer_train_id == ph_send.retransfer_train_id
                                && last_ph.paket_id == (ph_send.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            log_zeile(f, last_recv_send, last_ph);

                            log_zeile(f, send_str, ph_send);

                        }
                    }

                    php_send = NULL;
                    last_ph = ph_send;
                    last_recv_send = last_send;
                    // send speichern
                }

            }

        } else {

            if (ph_recv.recv_time.tv_sec < ph_send.send_time.tv_sec) {

                {
                    // recv speichern
                    if (last_ph.train_id == -1) {

                        log_zeile(f, recv_str, ph_recv);

                    } else {

                        if (last_ph.train_id == ph_recv.train_id
                                && last_ph.retransfer_train_id == ph_recv.retransfer_train_id
                                && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            log_zeile(f, last_recv_send, last_ph);

                            log_zeile(f, recv_str, ph_recv);

                        }
                    }

                    php_recv = NULL;
                    last_ph = ph_recv;
                    last_recv_send = last_recv;
                    // recv speichern
                }

            } else if (ph_send.send_time.tv_sec < ph_recv.recv_time.tv_sec) {

                {
                    // send speichern
                    if (last_ph.train_id == -1) {

                        log_zeile(f, send_str, ph_send);

                    } else {

                        if (last_ph.train_id == ph_send.train_id
                                && last_ph.retransfer_train_id == ph_send.retransfer_train_id
                                && last_ph.paket_id == (ph_send.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            log_zeile(f, last_recv_send, last_ph);

                            log_zeile(f, send_str, ph_send);

                        }
                    }

                    php_send = NULL;
                    last_ph = ph_send;
                    last_recv_send = last_send;
                    // send speichern
                }

            } else {

                if (ph_recv.recv_time.tv_nsec < ph_send.send_time.tv_nsec) {

                    {
                        // recv speichern
                        if (last_ph.train_id == -1) {

                            log_zeile(f, recv_str, ph_recv);

                        } else {

                            if (last_ph.train_id == ph_recv.train_id
                                    && last_ph.retransfer_train_id == ph_recv.retransfer_train_id
                                    && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                            } else {

                                fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                                fflush(f);

                                log_zeile(f, last_recv_send, last_ph);

                                log_zeile(f, recv_str, ph_recv);

                            }
                        }

                        php_recv = NULL;
                        last_ph = ph_recv;
                        last_recv_send = last_recv;
                        // recv speichern
                    }

                } else {

                    {
                        // send speichern
                        if (last_ph.train_id == -1) {

                            log_zeile(f, send_str, ph_send);

                        } else {

                            if (last_ph.train_id == ph_send.train_id
                                    && last_ph.retransfer_train_id == ph_send.retransfer_train_id
                                    && last_ph.paket_id == (ph_send.paket_id - 1)) {

                            } else {

                                fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                                fflush(f);

                                log_zeile(f, last_recv_send, last_ph);

                                log_zeile(f, send_str, ph_send);

                            }
                        }

                        php_send = NULL;
                        last_ph = ph_send;
                        last_recv_send = last_send;
                        // send speichern
                    }

                }
            }
        }
    }
}

int main(int argc, char**argv) {

    DIR *dir;
    struct dirent *dirzeiger;

    /* das Verzeichnis öffnen */
    char pfad[] = "../../log_data/";
    int pfad_len = strlen(pfad);
    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }

    /* das komplette Verzeichnis auslesen */
    while ((dirzeiger = readdir(dir)) != NULL) {
        //        printf("%s\n", (*dirzeiger).d_name);
        char *c = (char*) malloc(strlen((*dirzeiger).d_name));
        strncpy(c, (*dirzeiger).d_name, strlen((*dirzeiger).d_name));
        DateiListe.push_back(c);
    }

    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }
    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }

    /* Dateien parsen */
    while ((dirzeiger = readdir(dir)) != NULL) {
        char *DateiName = (*dirzeiger).d_name;

        int DateiName_len = strlen(DateiName);
        char DateiNameEnde[] = "_recv.b";
        int DateiNameEnde_len = strlen(DateiNameEnde);

        if ((DateiNameEnde_len + 5) < DateiName_len) {
            if (0 == strcmp(DateiName + DateiName_len - DateiNameEnde_len, DateiNameEnde)) {

                char DateiName2[1024];
                strncpy(DateiName2, DateiName, DateiName_len - DateiNameEnde_len);

                char DateiNameEnde2[] = "_send.b";
                int DateiNameEnde2_len = strlen(DateiNameEnde2);

                strncpy(DateiName2 + DateiName_len - DateiNameEnde_len, DateiNameEnde2, DateiNameEnde2_len);

                DateiName2[DateiName_len - DateiNameEnde_len + DateiNameEnde2_len] = 0;

                if (istInDateiListe(DateiName2)) {
                    char DateiName3[1024];
                    strncpy(DateiName3, DateiName, DateiName_len - DateiNameEnde_len);

                    char DateiNameEnde3[] = ".csv";
                    int DateiNameEnde3_len = strlen(DateiNameEnde3);

                    strncpy(DateiName3 + DateiName_len - DateiNameEnde_len, DateiNameEnde3, DateiNameEnde3_len);

                    DateiName3[DateiName_len - DateiNameEnde_len + DateiNameEnde3_len] = 0;

                    if (istInDateiListe(DateiName3) == false) {
                        char d[pfad_len + DateiName_len + DateiNameEnde_len];
                        char d2[pfad_len + DateiName_len + DateiNameEnde2_len];
                        char d3[pfad_len + DateiName_len + DateiNameEnde3_len];

                        d[0] = 0;
                        d2[0] = 0;
                        d3[0] = 0;

                        char *p = d;
                        char *p2 = d2;
                        char *p3 = d3;

                        strncat(d, pfad, pfad_len);
                        strncat(d, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d, DateiNameEnde, DateiNameEnde_len);

                        strncat(d2, pfad, pfad_len);
                        strncat(d2, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d2, DateiNameEnde2, DateiNameEnde2_len);

                        strncat(d3, pfad, pfad_len);
                        strncat(d3, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d3, DateiNameEnde3, DateiNameEnde3_len);

                        create_csv_datei(d, d2, d3);

                        printf("Datei erstellt: %s \n", d3);
                    }
                }
            }
        }
    }

    /* Lesezeiger wieder schließen */
    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }

    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }
    /* Dateien parsen zus*/
    while ((dirzeiger = readdir(dir)) != NULL) {
        char *DateiName = (*dirzeiger).d_name;

        int DateiName_len = strlen(DateiName);
        char DateiNameEnde[] = "_recv.b";
        int DateiNameEnde_len = strlen(DateiNameEnde);

        if ((DateiNameEnde_len + 5) < DateiName_len) {
            if (0 == strcmp(DateiName + DateiName_len - DateiNameEnde_len, DateiNameEnde)) {

                char DateiName2[1024];
                strncpy(DateiName2, DateiName, DateiName_len - DateiNameEnde_len);

                char DateiNameEnde2[] = "_send.b";
                int DateiNameEnde2_len = strlen(DateiNameEnde2);

                strncpy(DateiName2 + DateiName_len - DateiNameEnde_len, DateiNameEnde2, DateiNameEnde2_len);

                DateiName2[DateiName_len - DateiNameEnde_len + DateiNameEnde2_len] = 0;

                if (istInDateiListe(DateiName2)) {
                    char DateiName3[1024];
                    strncpy(DateiName3, DateiName, DateiName_len - DateiNameEnde_len);

                    char DateiNameEnde3[] = "_zus.csv";
                    int DateiNameEnde3_len = strlen(DateiNameEnde3);

                    strncpy(DateiName3 + DateiName_len - DateiNameEnde_len, DateiNameEnde3, DateiNameEnde3_len);

                    DateiName3[DateiName_len - DateiNameEnde_len + DateiNameEnde3_len] = 0;

                    if (istInDateiListe(DateiName3) == false) {
                        char d[pfad_len + DateiName_len + DateiNameEnde_len];
                        char d2[pfad_len + DateiName_len + DateiNameEnde2_len];
                        char d3[pfad_len + DateiName_len + DateiNameEnde3_len];

                        d[0] = 0;
                        d2[0] = 0;
                        d3[0] = 0;

                        char *p = d;
                        char *p2 = d2;
                        char *p3 = d3;

                        strncat(d, pfad, pfad_len);
                        strncat(d, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d, DateiNameEnde, DateiNameEnde_len);

                        strncat(d2, pfad, pfad_len);
                        strncat(d2, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d2, DateiNameEnde2, DateiNameEnde2_len);

                        strncat(d3, pfad, pfad_len);
                        strncat(d3, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d3, DateiNameEnde3, DateiNameEnde3_len);

                        create_csv_datei_zus(d, d2, d3);

                        printf("Datei erstellt: %s \n", d3);
                    }
                }

            }
        }

    }

    /* Lesezeiger wieder schließen */
    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }

    return EXIT_SUCCESS;


}