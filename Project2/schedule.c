#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <math.h>

#define MAX_SIZE 1000

struct burst
{
    int bduration; // burst duration
    int arrivalt;
    int turnaroundt;
};
double fcfs(struct burst olist[]);
double sjf(struct burst olist[]);
double srtf(struct burst olist[]);
double rr(struct burst olist[], int q);

//FCFS first come first served
double fcfs(struct burst olist[])
{
    int time = 0;
    double counter = 0, totalturnaround = 0;
    struct burst list[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
    {
        list[i].bduration = olist[i].bduration;
        list[i].arrivalt = olist[i].arrivalt;
        list[i].turnaroundt = olist[i].turnaroundt;
    }

    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (list[i].bduration == -1)
        {
            break;
        }

        if (list[i].arrivalt > time)
        {
            while (list[i].arrivalt > time)
            {
                time++;
            }
        }

        time += list[i].bduration;
        list[i].turnaroundt = time - list[i].arrivalt;
        list[i].bduration = -1;
        counter++;
    }
    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (olist[i].bduration != -1)
            totalturnaround += (double)list[i].turnaroundt;
        else
            break;
    }
    return totalturnaround / (counter);
}

//SJF shortest job first
double sjf(struct burst olist[])
{
    int time = 0, index = 0, minburst = 500;
    double counter = 0, totalturnaround = 0;
    struct burst list[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
    {
        list[i].bduration = olist[i].bduration;
        list[i].arrivalt = olist[i].arrivalt;
        list[i].turnaroundt = olist[i].turnaroundt;
    }

    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (list[index].arrivalt > time)
        {
            while (list[index].arrivalt > time)
            {
                time++;
            }
        }

        for (int j = 0; j < MAX_SIZE; j++)
        {
            if (list[j].arrivalt <= time)
            {
                if (list[j].bduration != -1 && list[j].bduration < minburst)
                {
                    index = j;
                    minburst = list[j].bduration;
                }
            }
            else
                break;
        }

        if (list[index].bduration != -1)
        {
            time += list[index].bduration;
            list[index].turnaroundt = time - list[index].arrivalt;
            list[index].bduration = -1;
            minburst = 500;
            counter++;
        }
        else
            time++;
    }
    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (olist[i].bduration != -1)
            totalturnaround += (double)list[i].turnaroundt;
        else
            break;
    }
    return totalturnaround / counter;
}

//SRTF shortest remaining time first
double srtf(struct burst olist[])
{
    int time = 0, isntOver = 1, index = 0, minburst = 500;
    double counter = 0, totalturnaround = 0;
    struct burst list[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
    {
        list[i].bduration = olist[i].bduration;
        list[i].arrivalt = olist[i].arrivalt;
        list[i].turnaroundt = olist[i].turnaroundt;
    }

    while (isntOver == 1)
    {
        isntOver = -1;
        if (list[index].arrivalt > time)
        {
            while (list[index].arrivalt > time)
            {
                time++;
            }
        }

        for (int i = 0; i < MAX_SIZE; i++)
        {
            if (list[i].arrivalt <= time)
            {
                if (list[i].bduration != -1 && list[i].bduration < minburst)
                {
                    index = i;
                    minburst = list[i].bduration;
                }
            }
            else
                break;
        }

        if (list[index].bduration != -1)
        {
            list[index].bduration--;
            minburst--;
            time++;
            if (list[index].bduration == 0)
            {
                list[index].turnaroundt = time;
                list[index].bduration = -1;
                counter++;
                minburst = 500;
            }
        }
        else
            time++;

        for (int i = 0; i < MAX_SIZE; i++)
        {
            if (list[i].bduration != -1)
            {
                isntOver = 1;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (olist[i].bduration != -1)
            totalturnaround += (double)(list[i].turnaroundt - list[i].arrivalt);
        else
            break;
    }
    return totalturnaround / counter;
}

//RR(q) round robin
double rr(struct burst olist[], int q)
{
    int time = 0, isntOver = 1;
    double counter = 0, totalturnaround = 0;
    struct burst list[MAX_SIZE];
    for (int i = 0; i < MAX_SIZE; i++)
    {
        list[i].bduration = olist[i].bduration;
        list[i].arrivalt = olist[i].arrivalt;
        list[i].turnaroundt = olist[i].turnaroundt;
    }

    while (isntOver == 1)
    {
        for (int i = 0; i < MAX_SIZE; i++)
        {
            isntOver = -1;
            if (list[i].arrivalt > time)
            {
                while (list[i].arrivalt > time)
                {
                    time++;
                }
            }

            if (list[i].bduration != -1)
            {
                if (list[i].bduration < q)
                {
                    time += list[i].bduration;
                    list[i].turnaroundt = time;
                    list[i].bduration = -1;
                    counter++;
                }
                else
                {
                    time += q;
                    list[i].bduration -= q;
                }
            }

            for (int j = 0; j < MAX_SIZE; j++)
            {
                if (list[j].bduration != -1)
                {
                    isntOver = 1;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_SIZE; i++)
    {
        if (olist[i].bduration != -1)
            totalturnaround += (double)(list[i].turnaroundt - list[i].arrivalt);
        else
            break;
    }
    return totalturnaround / counter;
}

int main(int argc, char **argv)
{

    char *p;
    int q = strtol(argv[2], &p, 10);
    double fcfsres, sjfres, srtfres, rrres;
    int ifcfsres, isjfres, isrtfres, irrres;
    char *filename = argv[1];
    struct burst list[MAX_SIZE];

    for (int i = 0; i < MAX_SIZE; i++)
    {
        list[i].arrivalt = -1;
        list[i].bduration = -1;
        list[i].turnaroundt = -1;
    }

    //creates arrays of bursts
    FILE *fp;
    char line[512];
    fp = fopen(filename, "r");

    //Controlling error while reading the file
    if (fp == NULL)
    {
        printf("There is a error while reading the file\n");
        exit(0);
    }

    int counter = 0;
    while (fgets(line, sizeof(line), fp))
    {
        for (int i = 0; i < (strlen(line) - 1); i++)
        {
            list[counter].arrivalt = 0;
            list[counter].bduration = 0;
            while (i < (strlen(line) - 1) && line[i] != ' ')
            {
                i++;
            }
            i++; //we pass the " "

            while (i < (strlen(line) - 1) && line[i] != ' ')
            {
                list[counter].arrivalt = list[counter].arrivalt * 10 + (line[i] - '0');
                i++;
            }
            i++; //we pass the " "
            while (i < (strlen(line) - 1) && line[i] != ' ')
            {
                list[counter].bduration = list[counter].bduration * 10 + (line[i] - '0');
                i++;
            }
        }
        counter++;
    }
    fclose(fp);

    fcfsres = fcfs(list);
    ifcfsres = (int)round(fcfsres);
    printf("FCFS: %d\n", ifcfsres);

    sjfres = sjf(list);
    isjfres = (int)round(sjfres);
    printf("SJF: %d\n", isjfres);

    srtfres = srtf(list);
    isrtfres = (int)round(srtfres);
    printf("STRF: %d\n", isrtfres);

    rrres = rr(list, q);
    irrres = (int)round(rrres);
    printf("RR: %d\n", irrres);

    return 0;
}
