# Plot microbenchmark results
# @prefix: path to results
# @form: output form (ie. "paper", "paper_sm", "pdf", "svg")
prefix <- "dummy_results/"
form <- "pdf"

# Returns list(mean, 95% C.I.)
confint95 <- function(x) {
	return(list(m=mean(x), e=qnorm(0.975)*sd(x)/sqrt(length(x))))
}

# CPU overhead barplot
cpu_overhead <- function(doplot=TRUE) {
	# Plotting parameters
	ffreqs <- c(0, 10, 20, 40);
	
	# Variables used while plotting
	means <- c();
	errors <- c();
	colors <- c();
	names <- c();
	ylim <- 5
	
	# Load the data
	#source(paste(prefix, "sosp_results.R", sep=""));
	source(paste(prefix, "nowait_results.R", sep=""));
	bline <- confint95(x0);

	# Get state-based measurements
	for (f in ffreqs) {
		eval(parse(text=paste("t <- confint95(s", f, ");", sep="")))
		means <- c(means, 1.0 - t$m/bline$m)
		errors <- c(errors, t$e/t$m)
		colors <- c(colors, "blue2")
		names <- c(names, paste(f, "ms", sep=""))
	}

	# Get event-based measurements
	for (f in ffreqs) {
		eval(parse(text=paste("t <- confint95(e", f, ");", sep="")))
		means <- c(means, 1.0 - t$m/bline$m)
		errors <- c(errors, t$e/t$m)
		colors <- c(colors, "firebrick3")
		names <- c(names, paste(f, "ms", sep=""))
	}

	means <- 100*means
	errors <- 100*errors
	
	if (doplot) {
		# Plotting output format (def: 4x2.675)
		if (form == "pdf")
			pdf(file="./cpu_overhead.pdf", width=8, height=5, pointsize=14)
		else if (form == "paper")
			pdf(file="./cpu_overhead.pdf", width=4, height=2.675, pointsize=10)
		else if (form == "paper_sm")
			pdf(file="./cpu_overhead.pdf", width=4, height=2, pointsize=10)
		else if (form == "svg")
			svg(filename="./cpu_overhead.svg", width=9,	height=5.5, pointsize=14)
	}

	par(mar=c(3.5, 3.5, 1, 1))
	mp <- barplot(means, names.arg=names, las=1, col=colors, ylim=c(0,ylim),
					 main = NULL, axes=F)
	
	# Now plot the labels, captions, and ablines
	axis(2, las=1, at=seq(0, ylim, by=.5), cex.axis=.8)
	mtext(text="Fetching frequency (ms)", side=1, line=2.5)
	mtext(text="CPU overhead of Duet (%)", side=2, line=2.5)
	abline(h=seq(0, ylim, by=.5), col="gray50", lty=3, lwd=2.0)
	
	# Plot the vertical lines of the error bars
	# The vertical bars are plotted at the midpoints
	ew <- 0.3
	segments(mp, means - errors, mp, means + errors, lwd=1.5)
	# Now plot the horizontal bounds for the error bars
	# 1. The lower bar
	segments(mp - ew, means - errors, mp + ew, means - errors, lwd=1.5)
	# 2. The upper bar
	segments(mp - ew, means + errors, mp + ew, means + errors, lwd=1.5)
	
	legend("topright", c("State-based", "Event-based"),
		   fill=c("blue2", "firebrick3"), inset=c(0.01, 0.01), cex=1, bty="b")
	
	if (doplot)
		dev.off()
}

cpu_overhead(TRUE)
