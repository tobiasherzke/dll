function [smoothed, e2] = dll(input, srate, fragsize, bandwidth)

F = srate/fragsize;
B = bandwidth;
w = 2*pi*B/F;
a = 0;
b = sqrt(2)*w;
c = w^2;

times = input;

tper = 1/F;
e2=tper;
t0 = times(1);
t1 = t0 + e2;
smoothed = [];
smoothed = [smoothed;t0];
for time = times(2:end)'
  e = time - t1;
  t0 = t1;
  smoothed = [smoothed;t0];
  t1 = t1 + b*e + e2;
  e2 = e2 + c*e;
end
