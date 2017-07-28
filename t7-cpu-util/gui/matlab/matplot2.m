ds=load('../experiment/pwr_Jul_28_2017-14.58.48.log');

%d=ds(47450:47550,:);
d=ds;

%d=load('LGN_30s_on_15000sps_scale100.log');
% 
% f = designfilt('lowpassfir', ...
%     'PassbandFrequency',0.15,'StopbandFrequency',0.2, ...
%     'PassbandRipple',1,'StopbandAttenuation',60, ...
%     'DesignMethod','equiripple');
% y = filtfilt(f,d(:,1));

%Somehow, some data are arrives before the previous one!!!
%d=sortrows(b,9); --> WRONG!!!

iA = d(:,1);
y = remove_spikes(iA,5000000);
% subplot(2,1,1); plot(iA); subplot(2,1,2); plot(x);
% return

%iA = y;
ts = d(:,2);
pA = zeros(length(d),1);
pY = zeros(length(d),1);
st = zeros(length(d),1); %sampling time in sec
t  = zeros(length(d),1);

for i=1:length(d)
%     pA(i) = ((iA(i)*4.1943/4194304)/(0.005*50))*0.76; %NOTE: direct V-meas~2.156
%     pY(i) = ((y(i)*4.1943/4194304)/(0.005*50))*0.76; %0.76 because we don't add magic number 100 / 167
    pA(i) = ((iA(i)*4.1943/4194304)/(0.005*50))*1.2; %NOTE: direct V-meas~2.156
    pY(i) = ((y(i)*4.1943/4194304)/(0.005*50))*1.2; %0.76 because we don't add magic number 100 / 167
    if i ~= 1
        st(i) = ts(i)-ts(i-1);
        t(i) = ts(i)-ts(1);
        %if(ts(i)<ts(i-1))
        %    disp('Found mis...')
        %end
    end
end

mt = mean(st);
for i=2:length(d)
    %t(i) = t(i-1)+mt;
end

%I[i] = (float(I1[i])*4.1943/4194304)/(0.005*50)
%        V[i] = (float(V1[i])*4.1943/4194304)
%        V33[i] = (float(V3[i])*4.1943/4194304)
f1=figure('Color','w');
plot(t,pA); title('Power Distribution Bank-A'); xlabel('Time (s)'); ylabel('Watt');
%xlim([0,tlim]);
f1=figure('Color','w');
plot(t,pY); title('Power Distribution Bank-A'); xlabel('Time (s)'); ylabel('Watt');
%xlim([0,tlim]);
fprintf('Reported sampling time = %f-msec\n',mean(st)*1000); 
t0=d(1,2); tn=d(length(d),2); s=(tn-t0)/length(d);
fprintf('Average sampling time = %f-msec\n',s*1000);
