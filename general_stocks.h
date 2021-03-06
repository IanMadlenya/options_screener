#ifndef _H_GENERAL_STOCKS
#define _H_GENERAL_STOCKS

#include "screener.h"

long historical_array_size;
long all_options_size;
struct historical_price **historical_price_array; // contains all historical price data from database, freed in gather_tickers
struct option **all_options;                      // contains all options info from database, freed in gather_options

// collecting historical prices
int historical_price_callback(void *NotUsed, int argc, char **argv, char **azColName); // done
struct parent_stock **gather_tickers(long *pa_size);                                   // done
void gather_data(void);                                                                // done

// historical price functionality
void average_perc_change(struct parent_stock *stock); // done
void perc_from_high_low(struct parent_stock *stock);  // done
void large_price_drop(struct parent_stock *stock);    // done
void price_trend(struct parent_stock *stock);         // done

// write function to find all stocks that have had a 7.5% drop or gain in one or two days the past 10 days
//

void find_curr_stock_price(struct parent_stock *stock); // done
void avg_stock_close(struct parent_stock *stock);       // done

#endif
