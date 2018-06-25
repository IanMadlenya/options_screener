#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>

#include "screener.h"
#include "util.h"
#include "safe.h"

long historical_array_size;
long all_options_size;
struct historical_price **historical_price_array; // contains all historical price data from database, freed in gather_tickers
struct option **all_options;                      // contains all options info from database, freed in gather_options

struct parent_stock **gather_tickers(long *pa_size)
{
   int i;
   char previous[TICK_SIZE];
   long price_array_size, parent_array_size;
   struct parent_stock **parent_array; // list of all stock tickers containing lists of their historical prices

   parent_array_size = 0;
   parent_array = safe_malloc(sizeof(struct parent_stock *));
   memset(previous, 0, TICK_SIZE);

   gather_data();

   for (i = 0; i < historical_array_size; i++)
   {
      if (0 != strcmp(historical_price_array[i]->ticker, previous))
      {
         memset(previous, 0, TICK_SIZE);
         strcpy(previous, historical_price_array[i]->ticker);

         parent_array = safe_realloc(parent_array, ++parent_array_size * (sizeof(struct parent_stock *)));
         parent_array[parent_array_size - 1] = safe_malloc(sizeof(struct parent_stock));

         memset(parent_array[parent_array_size - 1]->ticker, 0, TICK_SIZE);
         strcpy(parent_array[parent_array_size - 1]->ticker, historical_price_array[i]->ticker);
         parent_array[parent_array_size - 1]->prices_array = safe_malloc(sizeof(struct historical_price *));

         price_array_size = 0;
      }

      parent_array[parent_array_size - 1]->prices_array = safe_realloc(parent_array[parent_array_size - 1]->prices_array, ++price_array_size * (sizeof(struct historical_price *)));
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1] = safe_malloc(sizeof(struct historical_price));
      parent_array[parent_array_size - 1]->price_array_size = price_array_size;

      strcpy(parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->ticker, historical_price_array[i]->ticker);
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->date = historical_price_array[i]->date;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->open = historical_price_array[i]->open;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->low = historical_price_array[i]->low;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->high = historical_price_array[i]->high;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->close = historical_price_array[i]->close;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->volume = historical_price_array[i]->volume;

      // to free up memory since this is memory intensive
      free(historical_price_array[i]);
   }

   free(historical_price_array);

   *pa_size = parent_array_size;
   return parent_array;
}

void gather_options(struct parent_stock **parent_array, long parent_array_size)
{
   int i, parent_array_index;
   char previous[TICK_SIZE];

   parent_array_index = 0;
   memset(previous, 0, TICK_SIZE);

   gather_options_data();

   // iterate through all_options array
   for (i = 0; i < all_options_size; i++)
   {
      // if it is a brand new ticker
      if (parent_array_index >= parent_array_size)
         printf("Something went wrong...\n");

      // if the current and previous ticker are different, means change in ticker
      if (0 != strcmp(all_options[i]->ticker, previous))
      {
         // to avoid skipping the first index
         if (i != 0)
            parent_array_index++;

         memset(previous, 0, TICK_SIZE);
         strcpy(previous, all_options[i]->ticker);

         parent_array[parent_array_index]->calls = safe_malloc(sizeof(struct option *));
         parent_array[parent_array_index]->puts = safe_malloc(sizeof(struct option *));
         parent_array[parent_array_index]->calls_size = 0;
         parent_array[parent_array_index]->puts_size = 0;
      }

      // if the option is a call
      if (all_options[i]->type == 1)
      {
         parent_array[parent_array_index]->calls = safe_realloc(parent_array[parent_array_index]->calls,
                                                                ++(parent_array[parent_array_index]->calls_size) * sizeof(struct option *));

         parent_array[parent_array_index]->calls[parent_array[parent_array_index]->calls_size - 1] = safe_malloc(sizeof(struct option));

         copy_option(parent_array[parent_array_index]->calls[parent_array[parent_array_index]->calls_size - 1], all_options[i]);
      }
      else if (all_options[i]->type == 0)
      {
         parent_array[parent_array_index]->puts = safe_realloc(parent_array[parent_array_index]->puts,
                                                               ++(parent_array[parent_array_index]->puts_size) * sizeof(struct option *));

         parent_array[parent_array_index]->puts[parent_array[parent_array_index]->puts_size - 1] = safe_malloc(sizeof(struct option));

         copy_option(parent_array[parent_array_index]->puts[parent_array[parent_array_index]->puts_size - 1], all_options[i]);
      }

      free(all_options[i]);
      // printf("%s\t%d\t%d\t%f\t%f\t%d\n", all_options[i]->ticker, all_options[i]->type, all_options[i]->days_til_expiration, all_options[i]->strike, all_options[i]->last_price, all_options[i]->in_the_money);
   }

   return;
}

void copy_option(struct option *new_option, struct option *old)
{
   memset(new_option->ticker, 0, 10);
   strcpy(new_option->ticker, old->ticker);

   new_option->type = old->type; // call if TRUE, put is FALSE
   new_option->expiration_date = old->expiration_date;
   new_option->days_til_expiration = old->days_til_expiration;
   new_option->strike = old->strike;
   new_option->volume = old->volume;
   new_option->open_interest = old->open_interest;
   new_option->bid = old->bid;
   new_option->ask = old->ask;
   new_option->last_price = old->last_price;
   new_option->percent_change = old->percent_change;
   new_option->in_the_money = old->in_the_money;
   new_option->implied_volatility = old->implied_volatility;
   new_option->iv20 = old->iv20;
   new_option->iv50 = old->iv50;
   new_option->iv100 = old->iv100;

   // protects against NULL values
   if (old->theta && old->beta && old->gamma && old->vega)
   {
      new_option->theta = old->theta;
      new_option->beta = old->beta;
      new_option->gamma = old->gamma;
      new_option->vega = old->vega;
   }
}

void gather_options_data(void)
{
   int rc;
   char *error_msg;
   sqlite3 *db;
   char *sql = "SELECT * FROM optionsData";

   rc = sqlite3_open("optionsData", &db);

   all_options_size = 0;
   all_options = malloc(sizeof(struct option *));

   if (rc != SQLITE_OK)
   {
      fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);

      return;
   }

   rc = sqlite3_exec(db, sql, options_callback, 0, &error_msg);

   sqlite3_close(db);

   return;
}

void gather_data(void)
{
   int rc;
   char *error_msg;
   sqlite3 *db;
   char *sql = "SELECT * FROM historicalPrices";

   historical_array_size = 0;
   historical_price_array = malloc(sizeof(struct historical_price *));

   rc = sqlite3_open("historicalPrices", &db);

   if (rc != SQLITE_OK)
   {
      fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);

      return;
   }

   rc = sqlite3_exec(db, sql, historical_price_callback, 0, &error_msg);

   sqlite3_close(db);

   return;
}

/* Effectively removes options from the list if the volume/open interest isn't up to standards */
void screen_volume_oi(struct parent_stock **parent_array, float parent_array_size)
{
   /*
    * If you come across something that must be removed, don't remove it. Simply
    * free all of the pointers associated with it and set the option struct to NULL,
    * then incorporate NULL checks for whenever you're iterating through the list
    */

   int outter_i, inner_i;

   for (outter_i = 0; outter_i < parent_array_size; outter_i++)
   {
      for (inner_i = 0; inner_i < parent_array[outter_i]->calls_size; inner_i++)
      {
      }

      for (inner_i = 0; inner_i < parent_array[outter_i]->puts_size; inner_i++)
      {
      }
   }

   return;
}

void price_trend(struct parent_stock stock)
{

   return;
}

void bid_ask_spread(struct option opt)
{

   return;
}

void perc_from_strike(struct option opt)
{

   return;
}

void yearly_high_low(struct parent_stock stock)
{

   return;
}

void perc_from_ivs(struct option opt)
{

   return;
}

void iv_range(struct option opt)
{

   return;
}

void avg_open_close(struct parent_stock stock)
{

   return;
}

void find_curr_price(struct parent_stock stock)
{

   return;
}

int historical_price_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
   historical_price_array = realloc(historical_price_array, ++historical_array_size * (sizeof(struct historical_price *)));
   historical_price_array[historical_array_size - 1] = malloc(sizeof(struct historical_price));

   memset(historical_price_array[historical_array_size - 1]->ticker, 0, 10);
   strcpy(historical_price_array[historical_array_size - 1]->ticker, argv[0]);
   historical_price_array[historical_array_size - 1]->date = atof(argv[1]);
   historical_price_array[historical_array_size - 1]->open = atof(argv[2]);
   historical_price_array[historical_array_size - 1]->low = atof(argv[3]);
   historical_price_array[historical_array_size - 1]->high = atof(argv[4]);
   historical_price_array[historical_array_size - 1]->close = atof(argv[5]);
   historical_price_array[historical_array_size - 1]->volume = atof(argv[6]);

   return 0;
}

int options_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
   all_options = realloc(all_options, ++all_options_size * (sizeof(struct option *)));
   all_options[all_options_size - 1] = malloc(sizeof(struct option));

   memset(all_options[all_options_size - 1]->ticker, 0, 10);
   strcpy(all_options[all_options_size - 1]->ticker, argv[0]);

   all_options[all_options_size - 1]->type = ((0 == strcmp(argv[1], "Call")) ? TRUE : FALSE); // call if TRUE, put is FALSE
   all_options[all_options_size - 1]->expiration_date = atof(argv[2]);
   all_options[all_options_size - 1]->days_til_expiration = atof(argv[3]);
   all_options[all_options_size - 1]->strike = atof(argv[4]);
   all_options[all_options_size - 1]->volume = atof(argv[5]);
   all_options[all_options_size - 1]->open_interest = atof(argv[6]);
   all_options[all_options_size - 1]->bid = atof(argv[7]);
   all_options[all_options_size - 1]->ask = atof(argv[8]);
   all_options[all_options_size - 1]->last_price = atof(argv[9]);
   all_options[all_options_size - 1]->percent_change = atof(argv[10]);
   all_options[all_options_size - 1]->in_the_money = (strcmp(argv[11], "True") ? TRUE : FALSE);
   all_options[all_options_size - 1]->implied_volatility = atof(argv[12]);
   all_options[all_options_size - 1]->iv20 = atof(argv[13]);
   all_options[all_options_size - 1]->iv50 = atof(argv[14]);
   all_options[all_options_size - 1]->iv100 = atof(argv[15]);

   // protects against NULL values
   if (argv[16] && argv[17] && argv[18] && argv[19])
   {
      all_options[all_options_size - 1]->theta = atof(argv[16]);
      all_options[all_options_size - 1]->beta = atof(argv[17]);
      all_options[all_options_size - 1]->gamma = atof(argv[18]);
      all_options[all_options_size - 1]->vega = atof(argv[19]);
   }

   return 0;
}