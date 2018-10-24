FILE* GnuPlotOpen();
int GnuPlot(FILE* gnuplot, float x0, float dX, double *buffer, int nbOfChannels, int nbSamples);
void GnuPlotClose(FILE* gnuplot);

int sendDataToGnuPlot(int handle, float x0, float dX, double *buffer, int nbOfChannels, int nbSamples);
