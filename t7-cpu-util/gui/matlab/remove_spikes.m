function dOut = remove_spikes(dIn, threshold)

z = dIn;
for i=2:length(z)
    if abs(z(i)-z(i-1))>threshold
        z(i) = z(i-1);
    end
end


dOut = z;

end


